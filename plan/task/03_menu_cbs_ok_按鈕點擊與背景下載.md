# 03_menu_cbs_ok_按鈕點擊與背景下載

本文件詳細規劃了點擊事件回呼與異步背景下載任務模組的程式碼編寫計畫，主要涉及 `menu/cbs/menu_cbs_ok.c` 檔案。

---

## 🎯 實作目標
1.  為選單上的「下載」選項註冊與繫結「點選確認 (OK)」時的觸發回呼函數。
2.  加入**單一下載限制鎖**，限制同一時間只能進行一個下載任務，若已有任務進行中則攔截並提示。
3.  點擊時，自動提取目標檔案的父資料夾，並在資料夾不存在時**遞迴建立資料夾**。
4.  呼叫內建異步背景傳輸系統進行檔案下載，並註冊專用的任務完成回呼。
5.  任務完成（成功、失敗或取消）時釋放單一鎖。

---

## 🛠️ 開發詳細指引

### 1. 宣告全域/靜態下載狀態鎖
*   **修改檔案**：[menu_cbs_ok.c](file:///d:/test/retroarch/menu/cbs/menu_cbs_ok.c)
*   **實作細節**：
    在 `menu_cbs_ok.c` 頂部適當位置宣告一個全局靜態布林變數，用來追蹤背景下載狀態。
```c
static bool g_playlist_download_in_progress = false;
```

---

### 2. 實作確認點擊回呼函數 `action_ok_playlist_entry_download`
*   **修改檔案**：[menu_cbs_ok.c](file:///d:/test/retroarch/menu/cbs/menu_cbs_ok.c)
*   **實作細節**：
    編寫當使用者在該項目按下「確認 / OK鍵（鍵盤 Enter 或手把 A 鍵）」時觸發的函數。

```c
static int action_ok_playlist_entry_download(const char *path,
      const char *label, unsigned type, size_t idx, size_t entry_idx)
{
   playlist_t *playlist               = playlist_get_cached();
   const struct playlist_entry *entry = NULL;
   char parent_dir[PATH_MAX_LENGTH];
   file_transfer_t *transf            = NULL;

   if (!playlist)
      return menu_cbs_exit();

   /* 1. 獲取當前選取的項目資訊 */
   playlist_get_index(playlist, entry_idx, &entry);
   if (!entry || !entry->download || !*entry->download)
      return menu_cbs_exit();

   /* 2. 併發下載檢查 (單一下載限制鎖) */
   if (g_playlist_download_in_progress)
   {
      /* 呼叫內建 OSD 通知，顯示已有下載正在進行 */
      runloop_msg_queue_push(
            "已有下載任務正在背景進行中，請等候完成。",
            1, 100, false, NULL, MESSAGE_QUEUE_ICON_DEFAULT,
            MESSAGE_QUEUE_CATEGORY_INFO);
      return menu_cbs_exit();
   }

   /* 3. 解析與遞迴建立父資料夾 */
   parent_dir[0] = '\0';
   fill_pathname_parent_dir(parent_dir, entry->path, sizeof(parent_dir));
   if (*parent_dir && !path_is_directory(parent_dir))
   {
      /* path_mkdir 內部在多數平台上已支援遞迴父資料夾建立 */
      path_mkdir(parent_dir);
   }

   /* 4. 上鎖 */
   g_playlist_download_in_progress = true;

   /* 5. 分配並配置下載傳輸參數 */
   transf = (file_transfer_t*)calloc(1, sizeof(*transf));
   if (!transf)
   {
      g_playlist_download_in_progress = false;
      return menu_cbs_exit();
   }

   /* 將目標儲存路徑拷貝至傳輸對象中 */
   strlcpy(transf->path, entry->path, sizeof(transf->path));

   /* 6. 呼叫背景任務並綁定完成時的回呼 */
   task_push_http_transfer_file(
         entry->download,
         false,           /* mute: 設為 false 以正常顯示下載進度提示 */
         "GET",
         playlist_entry_download_callback, /* 完成時的回呼函數 */
         transf);

   return menu_cbs_exit();
}
```

---

### 3. 實作任務結束回呼 `playlist_entry_download_callback`
*   **修改檔案**：[menu_cbs_ok.c](file:///d:/test/retroarch/menu/cbs/menu_cbs_ok.c)
*   **實作細節**：
    此函數當下載傳輸成功、失敗或中斷時，會由 `task_queue` 隊列的主執行緒自動調用。我們必須在此重置鎖，並做必要清理。

```c
static void playlist_entry_download_callback(
      retro_task_t *task, void *task_data,
      void *user_data, const char *error)
{
   file_transfer_t *transf = (file_transfer_t*)user_data;

   /* 1. 釋放單一下載鎖，允許使用者發起新下載 */
   g_playlist_download_in_progress = false;

   /* 2. 釋放傳輸物件的記憶體 */
   if (transf)
      free(transf);

   /* 註：RetroArch 內建的 task_push_http_transfer_file 
    * 會自動偵測 error 參數並透過 OSD 顯示下載成功或失敗通知。 */
}
```

---

### 4. 註冊與綁定 OK 動作事件
*   **修改檔案**：[menu_cbs_ok.c](file:///d:/test/retroarch/menu/cbs/menu_cbs_ok.c)
*   **實作細節**：
    在 `menu_cbs_init_bind_ok` 函數中，找到大約 `case FILE_TYPE_PLAYLIST_ENTRY:` 或針對我們新增的 `MENU_ENUM_LABEL_PLAYLIST_ENTRY_DOWNLOAD` 進行綁定。

```c
case MENU_ENUM_LABEL_PLAYLIST_ENTRY_DOWNLOAD:
   BIND_ACTION_OK(cbs, action_ok_playlist_entry_download);
   break;
```

---

## 📝 進度檢核表 (Sub-Checklist)

- [ ] **1. 下載鎖宣告**
  - [ ] 於 `menu_cbs_ok.c` 頂部宣告 `g_playlist_download_in_progress` 並初始化為 `false`。
- [ ] **2. 實作 OK 按鈕確認回呼**
  - [ ] 撰寫 `action_ok_playlist_entry_download` 函數。
  - [ ] 在函數內調用 `playlist_get_index` 解析出對應的 `playlist_entry`。
- [ ] **3. 實作單工攔截與資料夾建立**
  - [ ] 加入 `g_playlist_download_in_progress` 判斷與 `runloop_msg_queue_push` 錯誤提示。
  - [ ] 調用 `fill_pathname_parent_dir` 與 `path_mkdir` 以確保寫入目標資料夾結構正常。
- [ ] **4. 異步下載整合**
  - [ ] 配置 `file_transfer_t` 的 `path`。
  - [ ] 呼叫 `task_push_http_transfer_file` 啟動背景傳輸任務。
- [ ] **5. 實作任務結束回呼**
  - [ ] 撰寫 `playlist_entry_download_callback` 釋放傳輸記憶體。
  - [ ] 將 `g_playlist_download_in_progress` 重設為 `false`。
- [ ] **6. 註冊綁定**
  - [ ] 在 `menu_cbs_init_bind_ok` 綁定 `MENU_ENUM_LABEL_PLAYLIST_ENTRY_DOWNLOAD` 與該回呼。
