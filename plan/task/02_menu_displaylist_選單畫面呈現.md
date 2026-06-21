# 02_menu_displaylist_選單畫面呈現

本文件詳細規劃了選單 UI 呈現模組的程式碼編寫計畫，主要涉及 `menu/menu_displaylist.c` 檔案。

---

## 🎯 實作目標
1.  取得當前選取的遊戲項目（`playlist_entry`），判斷是否包含有效的 `download` 網址。
2.  判斷本地路徑（`path`）是否已存在實體檔案。
3.  當「具有下載網址」且「檔案在硬碟中不存在」時，將「下載遊戲」選項附加至遊戲操作子選單中（與執行、刪除等項目並列）。

---

## 🛠️ 開發詳細指引

### 1. 修改主要顯示列表解析函數
*   **修改檔案**：[menu_displaylist.c](file:///d:/test/retroarch/menu/menu_displaylist.c)
*   **修改函數**：`menu_displaylist_parse_horizontal_content_actions(menu_handle_t *menu, settings_t *settings, file_list_t *list)`
*   **實作細節**：
    該函數用於解析並呈現單個播放列表項目的操作選單。在大約第 3898 行，我們可以看到「執行 (Run)」按鈕的附加邏輯。我們應該在此區塊之後，或者在 Kiosk Mode 判斷的合適區塊插入我們的新選單項。

#### 顯示判定與新增邏輯：
```c
/* 取得當前 entry 的選擇索引 */
unsigned idx = menu->rpl_entry_selection_ptr;
const struct playlist_entry *entry = NULL;

if (playlist)
   playlist_get_index(playlist, idx, &entry);

/* 1. 檢查是否有下載連結 */
bool has_download = (entry && entry->download && *entry->download);

/* 2. 檢查本地檔案是否已存在 */
bool file_exists = false;
if (entry && entry->path && *entry->path)
{
   /* 使用內建的 path_is_valid() 來驗證實體檔案是否存在 */
   file_exists = path_is_valid(entry->path);
}

/* 3. 當有下載連結且實體檔案尚未存在時，顯示「下載」按鈕 */
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

## 📝 進度檢核表 (Sub-Checklist)

- [ ] **1. 理解與定位顯示列表函數**
  - [ ] 於 `menu_displaylist.c` 中定位 `menu_displaylist_parse_horizontal_content_actions` 函數。
  - [ ] 尋找附加「執行 (MENU_ENUM_LABEL_RUN)」項目的程式碼段落。
- [ ] **2. 實作條件判定**
  - [ ] 新增 `has_download` 變數，用於判斷 `entry->download` 的非空狀態。
  - [ ] 新增 `file_exists` 變數，並調用 `path_is_valid(entry->path)` 檢查檔案實體。
- [ ] **3. 附加選單選項**
  - [ ] 呼叫 `menu_entries_append` 附加全新定義的 `MENU_ENUM_LABEL_PLAYLIST_ENTRY_DOWNLOAD` 標記。
  - [ ] 將 `FILE_TYPE_PLAYLIST_ENTRY` 設為其類型，並將 `idx` 傳入做為選取索引。
- [ ] **4. 單體驗證**
  - [ ] 啟動 RetroArch 並導航至測試遊戲項目，當本地對應 ROM 已手動刪除且 `.lpl` 有下載連結時，確認能看見新增的「下載遊戲」選項。
  - [ ] 當手動放置 ROM 檔案至該路徑後，確認「下載遊戲」選項在重啟或重新進入該頁面時已自動隱藏。
