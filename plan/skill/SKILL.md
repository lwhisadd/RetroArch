---
name: retroarch-playlist-download
description: 在 RetroArch 中擴充播放清單（.lpl）自訂欄位，並實作動態選單 UI、多國語系以及異步背景下載任務的核心開發工作流。
---

# RetroArch 播放清單擴充與異步背景下載開發指南

本 Skill 彙整了在 RetroArch 框架中「擴充播放清單自訂欄位，並實作對應選單 UI 與異步背景下載任務」的完整標準作業流程（SOP）。可用於未來在 RetroArch 中擴充類似自訂選單操作或網路檔案傳輸功能時重覆使用。

---

## 📌 開發階段一：播放清單資料處理（Playlist Data Processing）

當需要為 `.lpl` 格式加入自訂屬性（例如：`"download"`）時，必須同時修改播放清單的宣告、記憶體生命週期、解析與寫入邏輯。

### 1. 修改 [playlist.h](file:///D:/test/retroarch/playlist.h)
*   **目標**：在 `struct playlist_entry` 結構體中，宣告指向字串的自訂指標欄位。
```c
struct playlist_entry
{
   /* ... 現有欄位 ... */
   char *download;
};
```

### 2. 修改 [playlist.c](file:///D:/test/retroarch/playlist.c)
*   **記憶體釋放**：在 `playlist_free_entry` 函數中，確認將自訂欄位指標安全 `free` 並指向 `NULL`。
*   **清單項目拷貝**：
    *   在 `playlist_update` 與 `playlist_push` 函數中，必須將 `entry->download` 指標拷貝至複製品（複製品需要使用 `stradd` 或 `strdup` 進行記憶體分配，避免複製品在清單排序或重組時遺失欄位資訊）。
*   **JSON 解析處理**：
    *   定位 `JSONObjectMemberHandler` 函數，在 JSON 物件解析分支中註冊 `"download"` 的字串提取與寫入，確保在讀取 `.lpl` 檔案時，自訂欄位能被自動解析。
*   **JSON 寫入處理**：
    *   定位 `playlist_write_file` 函數，在寫入 JSON 結構的區塊中，使用 `JSON_WriteMemberString` 將 `"download"` 字串與數值一併序列化寫回檔案。

---

## 📌 開發階段二：多國語系常數配置（Multi-language & Localization）

選單按鈕與說明文字切勿 hardcode。必須於系統底層語系對照常數中註冊，以原生支援多國語系。

### 1. 註冊標籤與說明常數 [msg_hash.h](file:///D:/test/retroarch/msg_hash.h)
*   在 `enum msg_hash_enums` 中，使用 `MENU_LABEL` 巨集新增常數：
```c
MENU_LABEL(PLAYLIST_ENTRY_DOWNLOAD),
```
*   這將在編譯時自動擴充生成 `MENU_ENUM_LABEL_PLAYLIST_ENTRY_DOWNLOAD`、`MENU_ENUM_SUBLABEL_PLAYLIST_ENTRY_DOWNLOAD` 與 `MENU_ENUM_LABEL_VALUE_PLAYLIST_ENTRY_DOWNLOAD` 三個常數。

### 2. 定義巨集常數字串 [msg_hash_lbl_str.h](file:///D:/test/retroarch/msg_hash_lbl_str.h)
*   定義與列舉綁定的巨集常數字串：
```c
#define MENU_ENUM_LABEL_PLAYLIST_ENTRY_DOWNLOAD_STR "playlist_entry_download"
```

### 3. 常數綁定與對照 [intl/msg_hash_lbl.h](file:///D:/test/retroarch/intl/msg_hash_lbl.h)
*   註冊常數與字串之關聯：
```c
MSG_HASH(
   MENU_ENUM_LABEL_PLAYLIST_ENTRY_DOWNLOAD,
   MENU_ENUM_LABEL_PLAYLIST_ENTRY_DOWNLOAD_STR
   )
```

### 4. 編寫語系定義檔
在個別語言的對照檔（如 `intl/msg_hash_us.h`、`intl/msg_hash_cht.h`、`intl/msg_hash_chs.h`）中填入實際翻譯與副標題：
```c
/* 例：intl/msg_hash_cht.h */
MSG_HASH(
   MENU_ENUM_LABEL_VALUE_PLAYLIST_ENTRY_DOWNLOAD,
   "下載遊戲"
   )
MSG_HASH(
   MENU_ENUM_SUBLABEL_PLAYLIST_ENTRY_DOWNLOAD,
   "從指定的網址下載遊戲檔案 (ROM)。"
   )
```

---

## 📌 開發階段三：選單 UI 動態呈現（Menu UI Presentation）

在子操作選單中，必須根據業務邏輯動態判斷並附加自訂選項按鈕。

### 1. 修改 [menu_displaylist.c](file:///D:/test/retroarch/menu/menu_displaylist.c)
*   定位至 `menu_displaylist_parse_horizontal_content_actions` 函數。
*   取得當前 `entry = playlist_get_index`。
*   撰寫判定條件（例如：是否有下載網址、且本地檔案是否不存在 `!path_is_valid(entry->path)`）。
*   若條件成立，調用 `menu_entries_append` 動態將選項添加至選單列表中。
```c
bool has_download = (entry && entry->download && *entry->download);
bool file_exists  = (entry && entry->path && *entry->path && path_is_valid(entry->path));

if (has_download && !file_exists)
{
   menu_entries_append(list,
         msg_hash_to_str(MENU_ENUM_LABEL_VALUE_PLAYLIST_ENTRY_DOWNLOAD),
         msg_hash_to_str(MENU_ENUM_LABEL_PLAYLIST_ENTRY_DOWNLOAD),
         MENU_ENUM_LABEL_PLAYLIST_ENTRY_DOWNLOAD,
         FILE_TYPE_PLAYLIST_ENTRY, 0, idx, NULL);
}
```

---

## 📌 開發階段四：動作點擊與異步背景傳輸（OK Click & Background Task）

當用戶選取該按鈕並按下 OK/A 鍵時，需由底層事件處理模組進行防重複下載鎖、路徑分析、遞迴目錄建立與異步背景傳輸的控制。

### 1. 修改 [menu_cbs_ok.c](file:///D:/test/retroarch/menu/cbs/menu_cbs_ok.c)

#### A. 宣告防併發重複下載鎖
在 callback 區塊宣告：
```c
static bool g_playlist_download_in_progress = false;
```

#### B. 實作任務結束回呼 `playlist_entry_download_callback`
下載結束不論成功、失敗或取消，皆應解鎖並釋放記憶體。
```c
static void playlist_entry_download_callback(
      retro_task_t *task, void *task_data,
      void *user_data, const char *error)
{
   file_transfer_t *transf = (file_transfer_t*)user_data;

   g_playlist_download_in_progress = false;

   if (transf)
      free(transf);
}
```

#### C. 實作確認點擊回呼 `action_ok_playlist_entry_download`
*   **下載鎖檢查**：已上鎖時，調用 `runloop_msg_queue_push` 彈出多國語系對照常數 `MENU_ENUM_LABEL_VALUE_QT_DOWNLOAD_ALREADY_IN_PROGRESS` 進行 OSD 衝突提示並返回。
*   **遞迴父目錄建立**：使用 `fill_pathname_parent_dir` 解析儲存路徑，並對不存在的目錄調用 `path_mkdir` 進行遞迴目錄自動建立。
*   **背景異步傳輸啟動**：
    *   分配 `file_transfer_t`，拷貝目標寫入路徑。
    *   呼叫 `task_push_http_transfer_file` 將 URL 推送至背景任務隊列，並與 callback 綁定。
```c
static int action_ok_playlist_entry_download(const char *path,
      const char *label, unsigned type, size_t idx, size_t entry_idx)
{
   playlist_t *playlist               = playlist_get_cached();
   const struct playlist_entry *entry = NULL;
   char parent_dir[PATH_MAX_LENGTH];
   file_transfer_t *transf            = NULL;

   if (!playlist)
      return -1;

   playlist_get_index(playlist, entry_idx, &entry);
   if (!entry || !entry->download || !*entry->download)
      return -1;

   if (g_playlist_download_in_progress)
   {
      const char *msg = msg_hash_to_str(MENU_ENUM_LABEL_VALUE_QT_DOWNLOAD_ALREADY_IN_PROGRESS);
      if (msg)
         runloop_msg_queue_push(msg, strlen(msg), 1, 100, false, NULL,
               MESSAGE_QUEUE_ICON_DEFAULT, MESSAGE_QUEUE_CATEGORY_INFO);
      return -1;
   }

   parent_dir[0] = '\0';
   fill_pathname_parent_dir(parent_dir, entry->path, sizeof(parent_dir));
   if (*parent_dir && !path_is_directory(parent_dir))
   {
      path_mkdir(parent_dir);
   }

   g_playlist_download_in_progress = true;

   transf = (file_transfer_t*)calloc(1, sizeof(*transf));
   if (!transf)
   {
      g_playlist_download_in_progress = false;
      return -1;
   }

   strlcpy(transf->path, entry->path, sizeof(transf->path));

   task_push_http_transfer_file(
         entry->download,
         false,
         "GET",
         playlist_entry_download_callback,
         transf);

   return 0;
}
```

#### D. 在 `ok_list` 靜態對照表中註冊綁定
定位至 `menu_cbs_init_bind_ok_compare_label` 函數，在其靜態 `ok_list[]` 陣列中加入綁定列：
```c
{MENU_ENUM_LABEL_PLAYLIST_ENTRY_DOWNLOAD,             action_ok_playlist_entry_download},
```

---

## ⚠️ 踩坑記錄與編譯問題檢討 (Troubleshooting & Lessons Learned)

### 1. 連結錯誤：`undefined reference to 'menu_cbs_exit'`
* **發生原因**：
  在實作 `action_ok_playlist_entry_download` 時，切勿從測試範例 `plan/skill/examples/playlist_download_example.c` 中直接複製用於模擬 Mock 的 `menu_cbs_exit()` 函數。該模擬函數在真實的 RetroArch 程式碼中並不存在，且實作時若無提供定義，將導致連結階段報錯：
  ```text
  undefined reference to `menu_cbs_exit`
  ```
* **標準規範 / 修正方式**：
  RetroArch 的選單按鈕點擊回呼函數（`action_ok_...`）其標準返回值類型為 `int`。
  應遵循標準規範，在遭遇無效檢查（例如 `!playlist`、`!entry`）或因併發鎖被阻擋時直接返回 **`-1`**，而成功遞交異步背景任務時則返回 **`0`**。不應引入並呼叫未定義的模擬退出常式。
