KAOPA
=====
## 架構簡介

### Server
 - 用戶帳號、餘額存放在 **data_server.txt**，啟動時會自動載入
 - 運行時，用戶資料存放在 **std::map&lt;string, User&gt; users** 當中
 - 用戶登入時，將設定該 **User** 的 IP 及 port 為用戶登陸的值，並將該用戶存放到 **std::map&lt;string, User*&gt; users_online** 的 **map** 當中
 - server 預設監聽 port 8889，並會在用戶接入時以 **std::thread** 開啟一支 **connection_process**
 - 輸入 **q** 可儲存用戶資訊至檔案，並結束 server instance

### Client
 - 可由第一個參數指定要連接 server 的 port
 - 介面使用 **NCurses** 函式庫
 - 啟動後依照指示連接 server
 - 啟動後會監聽本地隨機 port，用於收款
 - **Client** 保存登入帳號名稱、本地餘額、本地 port、以及 **std::map&lt;string, User&gt; users** 用於存放 **List** 所取得的資訊
 - **Pay 發起付款** 可以付款給另一位在線用戶，會檢查輸入的用戶是否在 **List** 裡面，以及我方的餘額是否足夠
 - **History 交易紀錄** 會列出本次登入中經歷的交易紀錄，包括收/付款人，以及金額
 - **List 用戶列表** 會向 server 要求現在的線上用戶列表，並一併更新自己的餘額資訊
 - **Info 系統資訊** 顯示本次連線的加密方法，以及 server 端的憑證資訊
 - 主畫面下方狀態列，左起為現在時間、現在氣象(資料來源為中央氣象局)、我的帳號資訊(資料來源為 **List**)
 - client 中含有 **Client** class，將 **Socket** 包裝起來，並提供例如 **login(string username)** 等方法供呼叫
 - 收款時，**payment_accept** 將處理相關連接。另一位用戶的連入將會被處理後以 **payment_ack** 方法告知server，並將結果寫入 **std::vector&lt;Transaction&gt; transactionHistory**

### Utility
 - 包含 server, client 共用之函式庫
 - socket 部分採用 POSIX socket，並包裝為 **Socket** class
 - 加密連線採用 OpenSSL，包裝為繼承 **Socket** 的 **SecureSocket** class
 - 僅會對 OpenSSL initialize 一次，並於每個 **SecureSocket** 物件產生屬於自己的 **SSL_CTX*** 及 **SSL***
 - 預設使用 **server.crt.pem** 及 **server.key.pem** 做為憑證
 - 定義 **Transaction** class 用於在函式間傳遞一筆付款的資訊
 - 不包含 **User** class，由於 client, server 所需的資訊有所不同
