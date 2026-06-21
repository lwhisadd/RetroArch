# 04_msg_hash_多國語系配置

本文件詳細規劃了選單中顯示語系與標籤對照模組的程式碼編寫計畫，主要涉及 `msg_hash.h` 與 `intl/` 目錄下的多國語系定義檔。

---

## 🎯 實作目標
1.  在主列舉標籤檔案中定義全新的「下載遊戲」按鈕識別常數。
2.  在預設語系（英文）中加入「Download Game」字串定義。
3.  在繁體中文語系中加入「下載遊戲」字串定義。
4.  在簡體中文語系中加入「下載遊戲」字串定義。

---

## 🛠️ 開發詳細指引

### 1. 修改主要標籤列舉 (`msg_hash.h`)
*   **修改檔案**：[msg_hash.h](file:///d:/test/retroarch/msg_hash.h)
*   **實作細節**：
    在 `enum msg_hash_enums` 的適當段落（通常與 `MENU_ENUM_LABEL_PLAYLIST_ENTRY_` 等系列標籤並列）新增以下兩個標籤常數：

```c
enum msg_hash_enums
{
   /* ... 現有常數 ... */
   
   MENU_ENUM_LABEL_PLAYLIST_ENTRY_DOWNLOAD,
   MENU_ENUM_LABEL_VALUE_PLAYLIST_ENTRY_DOWNLOAD,

   /* ... 現有常數 ... */
};
```

---

### 2. 修改預設英文語系對照表 (`intl/msg_hash_us.h`)
*   **修改檔案**：[intl/msg_hash_us.h](file:///d:/test/retroarch/intl/msg_hash_us.h) (或 `intl/msg_hash_us.c`)
*   **實作細節**：
    在對照表巨集定義中加入以下對照值：

```c
#define MENU_ENUM_LABEL_PLAYLIST_ENTRY_DOWNLOAD_STR "playlist_entry_download"

/* 在對應的巨集或陣列中新增 */
MSG_HASH(MENU_ENUM_LABEL_PLAYLIST_ENTRY_DOWNLOAD, "playlist_entry_download")
MSG_HASH(MENU_ENUM_LABEL_VALUE_PLAYLIST_ENTRY_DOWNLOAD, "Download Game")
```

---

### 3. 修改繁體中文語系對照表 (`intl/msg_hash_zh_tw.h`)
*   **修改檔案**：[intl/msg_hash_zh_tw.h](file:///d:/test/retroarch/intl/msg_hash_zh_tw.h)
*   **實作細節**：
    在對照表巨集中，加入符合您喜好的翻譯，此處採用繁體中文「下載遊戲」：

```c
MSG_HASH(MENU_ENUM_LABEL_VALUE_PLAYLIST_ENTRY_DOWNLOAD, "下載遊戲")
```

---

### 4. 修改簡體中文語系對照表 (`intl/msg_hash_zh_cn.h`)
*   **修改檔案**：[intl/msg_hash_zh_cn.h](file:///d:/test/retroarch/intl/msg_hash_zh_cn.h)
*   **實作細節**：
    加入簡體中文「下載游戏」：

```c
MSG_HASH(MENU_ENUM_LABEL_VALUE_PLAYLIST_ENTRY_DOWNLOAD, "下載游戏")
```

---

## 📝 進度檢核表 (Sub-Checklist)

- [ ] **1. 列舉常數定義**
  - [ ] 於 `msg_hash.h` 的 `enum msg_hash_enums` 新增 `MENU_ENUM_LABEL_PLAYLIST_ENTRY_DOWNLOAD`。
  - [ ] 新增 `MENU_ENUM_LABEL_VALUE_PLAYLIST_ENTRY_DOWNLOAD`。
- [ ] **2. 預設英文語系對照**
  - [ ] 於 `intl/msg_hash_us.h` 加入 `MENU_ENUM_LABEL_PLAYLIST_ENTRY_DOWNLOAD_STR` 的巨集定義。
  - [ ] 綁定 `MENU_ENUM_LABEL_PLAYLIST_ENTRY_DOWNLOAD` 與英文字串值。
  - [ ] 綁定 `MENU_ENUM_LABEL_VALUE_PLAYLIST_ENTRY_DOWNLOAD` 為 `"Download Game"`。
- [ ] **3. 繁體中文語系配置**
  - [ ] 於 `intl/msg_hash_zh_tw.h` 加入 `MENU_ENUM_LABEL_VALUE_PLAYLIST_ENTRY_DOWNLOAD` 的對照。
  - [ ] 綁定顯示值為 `"下載遊戲"`。
- [ ] **4. 簡體中文語系配置**
  - [ ] 於 `intl/msg_hash_zh_cn.h` 加入 `MENU_ENUM_LABEL_VALUE_PLAYLIST_ENTRY_DOWNLOAD` 的對照。
  - [ ] 綁定顯示值為 `"下載游戏"`（或 `"下载游戏"`）。
- [ ] **5. 單體驗證**
  - [ ] 編譯專案並將 RetroArch 語系切換至「繁體中文」，確認當選項出現時，顯示文字正確無誤地呈現「下載遊戲」而非未解析的內部標記。
  - [ ] 切換至「English」確認正確顯示為「Download Game」。
