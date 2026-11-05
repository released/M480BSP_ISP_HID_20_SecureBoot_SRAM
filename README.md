# M487 Secure Boot：APROM／SRAM 更新入口

本範例由 APROM 接收更新要求，再以 SRAM 的向量、程式與 stack 呼叫晶片內建 Mask ROM Secure ISP API，搭配 **[0213 0921] SecureBootTool.exe** 更新 APROM。測試重點是更新期間不依賴 LDROM，且新版 APP 仍能再次進入更新模式。

**完整圖解與操作：[Secure Boot ISP 操作投影片](https://released.github.io/slide_Nuvoton_secure_boot_operation_M480/)**。Secure Boot 的開機驗證、SecureBootTool 的授權更新、ICP／金鑰配置及 SRAM 交接細節，集中於投影片。

## 建置

使用 M487JIDAE／NuMaker-PFM-M487 V3.0、Keil uVision 5／Arm Compiler 6.22。開啟 [SecureBootSRAM](SampleCode/AP/Keil/SecureBootSRAM.uvprojx)，選擇同名 target 後按 **F7（Build）**。

輸出：**`SampleCode/AP/Keil/obj/secure_boot_sram.bin`**，不產生 LDROM BIN。確認建置零錯誤／零警告，再使用 BIN 更新；F7 不會下載到 MCU。

[建置環境與配置](https://released.github.io/slide_Nuvoton_secure_boot_operation_M480/#build-examples)。

## 快速驗證

1. 依 [ICP 配置](https://released.github.io/slide_Nuvoton_secure_boot_operation_M480/#icp-setup) 準備 CONFIG、KPROM 與 SBK，正常目的地為 **APROM（CONFIG0[7]=1）**；已有相同配置時沿用現有金鑰。
2. 從板上已可用的 Secure ISP 入口連線；若 APROM／LDROM 均空白，使用配置後的 ROM 首次入口。PC 接 **CON1 HSUSB**，完成 KPROM 認證及 Key Options。[SRAM 首次部署](https://released.github.io/slide_Nuvoton_secure_boot_operation_M480/#sram-operation)。
3. Programming **只勾 APROM**，寫入 `secure_boot_sram.bin`；RESET／重新上電，確認 UART 出現 **Secure update SRAM** 及 APP 輸出。
4. 按 **SW2 至少 30 ms**，或在 **UART0 terminal 按數字 1（KEY1）**，再次連線認證。[按鍵與 terminal 設定](https://released.github.io/slide_Nuvoton_secure_boot_operation_M480/#entry-triggers)。
5. 只更新含相同 SRAM 模組與入口的新版 APROM；重置後核對新版輸出，並再次測試入口。[SRAM 更新流程與配置限制](https://released.github.io/slide_Nuvoton_secure_boot_operation_M480/#sram-operation)。

**注意事項：** 保留專案的 SRAM／IB／linker 配置與每版 APP 的更新模組。本例操作範圍為 APROM 更新；入口需要 APP 可運行，單槽更新不提供斷電回復保證。

錯誤狀態與恢復限制見投影片的 [排查表](https://released.github.io/slide_Nuvoton_secure_boot_operation_M480/#troubleshooting)。
