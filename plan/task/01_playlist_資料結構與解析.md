# 01_playlist_資料結構與解析

本文件詳細規劃了播放清單資料處理（`.lpl`）模組的程式碼編寫計畫，主要涉及 `playlist.h` 與 `playlist.c` 檔案。

---

## 🎯 實作目標
1.  擴充播放列表項目的結構體，使其支援讀取、儲存和釋放獨立的 `"download"` 網址字串。
2.  擴充 JSON 解析器，使其能正確讀取 `.lpl` 中的 `"download"` 鍵值。
3.  擴充播放列表寫入邏輯，使其在存檔時將 `"download"` 寫回檔案。

---

## 🛠️ 開發詳細指引

### 1. 擴充結構體 (`playlist.h`)
*   **修改檔案**：[playlist.h](file:///d:/test/retroarch/playlist.h)
*   **實作細節**：
    在 `struct playlist_entry` 結構體定義中（大約第 140 行上下），新增 `char *download;` 欄位。

```c
struct playlist_entry
{
   char *path;
   char *label;
   char *core_path;
   char *core_name;
   char *db_name;
   char *crc32;
   char *subsystem_ident;
   char *subsystem_name;
   char *runtime_str;
   char *last_played_str;
   struct string_list *subsystem_roms;
   playlist_path_id_t *path_id;
   unsigned entry_slot;
   unsigned runtime_hours;
   unsigned runtime_minutes;
   unsigned runtime_seconds;
   unsigned last_played_year;
   unsigned last_played_month;
   unsigned last_played_day;
   unsigned last_played_hour;
   unsigned last_played_minute;
   unsigned last_played_second;
   enum playlist_runtime_status runtime_status;
   int thumbnail_flags;
   
   char *download; /* 新增：下載連結網址 */
};
```

---

### 2. 記憶體管理與釋放 (`playlist.c`)
*   **修改檔案**：[playlist.c](file:///d:/test/retroarch/playlist.c)
*   **實作細節**：
    *   在結構釋放函數 `playlist_free_entry(struct playlist_entry *entry)` 中釋放 `download` 記憶體。
    *   在釋放後，將 `entry->download` 指標重設為 `NULL`。

```c
static void playlist_free_entry(struct playlist_entry *entry)
{
   if (!entry)
      return;

   /* ... 釋放現有的其他欄位 ... */

   if (entry->download)
      free(entry->download);
   
   /* ... 重設其他指標 ... */
   entry->download           = NULL;
}
```

---

### 3. JSON 格式解析 (`playlist.c`)
*   **修改檔案**：[playlist.c](file:///d:/test/retroarch/playlist.c)
*   **實作細節**：
    在 JSON 鍵值解析回呼函數 `JSONObjectMemberHandler(void *context, const char *pValue, size_t len)` 中：
    *   定位至 `switch (pValue[0])` 的 `case 'd'` 分支（大約第 2437 行）。
    *   新增對於 `"download"` 字串的配對與繫結。

```c
case 'd':
   if (!strcmp(pValue, "db_name"))
      pCtx->current_string_val = &pCtx->current_entry->db_name;
   else if (!strcmp(pValue, "download"))
      pCtx->current_string_val = &pCtx->current_entry->download; /* 新增 */
   break;
```

---

### 4. JSON 寫入更新 (`playlist.c`)
*   **修改檔案**：[playlist.c](file:///d:/test/retroarch/playlist.c)
*   **實作細節**：
    在 `playlist_write_file` 函數寫入 JSON 結構體欄位時：
    *   檢查當前寫入的 entry 其 `download` 是否不為 NULL 且不為空字串。
    *   如果是，則在寫入最後一項欄位後，附加一個逗號與 `"download"` 屬性值。

```c
if (playlist->entries[i].download && *playlist->entries[i].download)
{
   rjsonwriter_raw(writer, ",\n", 2);
   rjsonwriter_add_spaces(writer, 6);
   rjsonwriter_add_string(writer, "download");
   rjsonwriter_raw(writer, ": ", 2);
   rjsonwriter_add_string(writer, playlist->entries[i].download);
}
```

---

## 📝 進度檢核表 (Sub-Checklist)

- [ ] **1. 擴充結構體 (`playlist.h`)**
  - [ ] 在 `struct playlist_entry` 中加入 `char *download;` 欄位。
- [ ] **2. 記憶體釋放管理 (`playlist.c`)**
  - [ ] 修改 `playlist_free_entry`，加入對 `entry->download` 的 `free()` 呼叫。
  - [ ] 於釋放邏輯結尾，將 `entry->download` 設為 `NULL`。
- [ ] **3. JSON 解析支援 (`playlist.c`)**
  - [ ] 修改 `JSONObjectMemberHandler` 的 `case 'd'`。
  - [ ] 綁定 `"download"` 鍵值至 `&pCtx->current_entry->download`。
- [ ] **4. JSON 寫入支援 (`playlist.c`)**
  - [ ] 修改 `playlist_write_file` 的寫入迴圈。
  - [ ] 支援在資料變更或寫入時，將非空的 `download` 欄位序列化寫回 `.lpl`。
- [ ] **5. 單體驗證**
  - [ ] 使用含有 `"download"` 欄位的測試 `.lpl` 進行載入，確認系統編譯並載入正常。
  - [ ] 進行一次選單設定重置，確認寫回檔案後的 `.lpl` 中仍完整保留 `"download"` 屬性。
