# Implementation Plan: TCAN1463-Q1 Simulator

## Overview

TCAN1463-Q1 CANトランシーバーICシミュレーターをC++で実装します。実装は、コアデータ構造とコンポーネントから始め、段階的に機能を追加し、各段階でテストを実施します。

## Tasks

- [x] 1. プロジェクト構造とコアデータ型の設定
  - C++プロジェクト構造を作成（include/, src/, test/ディレクトリ）
  - CMakeLists.txtを作成してビルドシステムを設定
  - コアデータ型（PinType, PinState, OperatingMode等）をヘッダーファイルに定義
  - RapidCheckテストフレームワークを統合
  - _Requirements: 1.1, 1.2, 1.3, 1.4, 2.1-2.6_

- [x] 2. Pin Managerの実装
  - [x] 2.1 Pin Managerクラスの実装
    - Pinクラスを実装（状態、電圧、方向、範囲を管理）
    - PinManagerクラスを実装（全ピンの管理）
    - ピン値の設定/取得メソッドを実装
    - 電圧範囲検証ロジックを実装
    - _Requirements: 1.1, 1.2, 1.3, 1.4, 1.5, 1.6_
  
  - [x] 2.2 Pin Manager用プロパティテストの作成
    - **Property 1: Pin voltage validation**
    - **Validates: Requirements 1.5**
  
  - [x] 2.3 Pin Manager用プロパティテストの作成
    - **Property 2: Pin value round-trip consistency**
    - **Validates: Requirements 1.6, 11.2**

- [x] 3. Timing Engineの実装
  - [x] 3.1 Timing Engineクラスの実装
    - TimingEngineクラスを実装（ナノ秒精度の時間管理）
    - 時間進行メソッドを実装
    - 遅延追加メソッドを実装
    - タイムアウト判定メソッドを実装
    - _Requirements: 9.1, 9.2, 9.3, 9.4, 9.5, 9.6_
  
  - [x] 3.2 Timing Engine用プロパティテストの作成
    - **Property 24: Simulation time monotonicity**
    - **Validates: Requirements 11.3**

- [x] 4. Power Monitorの実装
  - [x] 4.1 Power Monitorクラスの実装
    - PowerStateデータ構造を実装
    - 電源電圧監視ロジックを実装
    - アンダーボルテージ検出（VSUP, VCC, VIO）を実装
    - フィルタタイミング（tUV）を実装
    - PWRONフラグ管理を実装
    - _Requirements: 4.1, 4.2, 4.3, 4.4, 4.5, 4.6, 4.7_
  
  - [x] 4.2 Power Monitor用プロパティテストの作成
    - **Property 6: Undervoltage detection and flag management**
    - **Validates: Requirements 4.1, 4.2, 4.3, 4.5, 4.6**
  
  - [x] 4.3 Power Monitor用プロパティテストの作成
    - **Property 7: PWRON flag on power-up**
    - **Validates: Requirements 4.4**
  
  - [x] 4.4 Power Monitor用プロパティテストの作成
    - **Property 8: Protected state on undervoltage**
    - **Validates: Requirements 4.7**

- [x] 5. Checkpoint - コア機能の検証
  - 全てのテストが成功することを確認
  - ユーザーに質問があれば確認

- [x] 6. Mode Controllerの実装
  - [x] 6.1 Mode Controllerクラスの実装
    - ModeStateデータ構造を実装
    - モード遷移テーブルを実装
    - モード遷移ロジックを実装（ピン状態、電源状態、フラグに基づく）
    - モード遷移検証メソッドを実装
    - _Requirements: 2.1, 2.2, 2.3, 2.4, 2.5, 2.6, 2.7, 2.8_
  
  - [x] 6.2 Mode Controller用プロパティテストの作成
    - **Property 3: Mode transition validity**
    - **Validates: Requirements 2.7**
  
  - [x] 6.3 Mode Controller用単体テストの作成
    - 各モードへの遷移をテスト
    - 無効な遷移が拒否されることをテスト
    - _Requirements: 2.1-2.6_

- [x] 7. CAN Transceiverの実装
  - [x] 7.1 CAN Transceiverクラスの実装
    - CANTransceiverデータ構造を実装
    - バス状態検出ロジック（差動電圧から）を実装
    - ドライバ制御（CANH/CANL駆動）を実装
    - レシーバ制御（RXD出力）を実装
    - CAN状態マシン（Off, Autonomous Inactive/Active, Active）を実装
    - _Requirements: 3.1, 3.2, 3.3, 3.4, 3.5, 3.6_
  
  - [x] 7.2 CAN Transceiver用プロパティテストの作成
    - **Property 4: CAN transceiver behavior consistency**
    - **Validates: Requirements 3.1, 3.2, 3.3, 3.4, 3.5**
  
  - [x] 7.3 CAN Transceiver用プロパティテストの作成
    - **Property 5: CAN FD data rate support**
    - **Validates: Requirements 3.6**

- [x] 8. Bus Bias Controllerの実装
  - [x] 8.1 Bus Bias Controllerクラスの実装
    - BusBiasControllerデータ構造を実装
    - バイアス状態マシンを実装
    - バイアス電圧計算を実装（VCC/2, 2.5V, GND, High-Z）
    - サイレンスタイムアウト検出を実装
    - _Requirements: 8.1, 8.2, 8.3, 8.4, 8.5, 8.6_
  
  - [x] 8.2 Bus Bias Controller用プロパティテストの作成
    - **Property 19: Bus bias control**
    - **Validates: Requirements 8.1, 8.2, 8.3, 8.4**
  
  - [x] 8.3 Bus Bias Controller用プロパティテストの作成
    - **Property 20: Autonomous state transition on silence**
    - **Validates: Requirements 8.5**
  
  - [x] 8.4 Bus Bias Controller用プロパティテストの作成
    - **Property 21: Autonomous state transition on wake-up**
    - **Validates: Requirements 8.6**

- [x] 9. Checkpoint - 通信機能の検証
  - 全てのテストが成功することを確認
  - ユーザーに質問があれば確認

- [x] 10. Fault Detectorの実装
  - [x] 10.1 Fault Detectorクラスの実装
    - FaultStateデータ構造を実装
    - TXDCLP検出（TXD clamped low）を実装
    - TXDDTO検出（TXD dominant timeout）を実装
    - TXDRXD検出（TXD/RXD short）を実装
    - CANDOM検出（CAN bus dominant fault）を実装
    - TSD検出（Thermal shutdown）を実装
    - CBF検出（CAN bus fault, 4連続遷移）を実装
    - nFAULT出力制御を実装
    - _Requirements: 5.1, 5.2, 5.3, 5.4, 5.5, 5.6, 5.7_
  
  - [x] 10.2 Fault Detector用プロパティテストの作成
    - **Property 9: TXD dominant timeout detection**
    - **Validates: Requirements 5.2**
  
  - [x] 10.3 Fault Detector用プロパティテストの作成
    - **Property 10: TXD/RXD short detection**
    - **Validates: Requirements 5.3**
  
  - [x] 10.4 Fault Detector用プロパティテストの作成
    - **Property 11: CAN bus dominant fault detection**
    - **Validates: Requirements 5.4**
  
  - [x] 10.5 Fault Detector用プロパティテストの作成
    - **Property 12: Thermal shutdown detection**
    - **Validates: Requirements 5.5**
  
  - [x] 10.6 Fault Detector用プロパティテストの作成
    - **Property 13: Fault indication on nFAULT**
    - **Validates: Requirements 5.7**
  
  - [x] 10.7 Fault Detector用単体テストの作成
    - TXDCLP、CBFなどのエッジケースをテスト
    - _Requirements: 5.1, 5.6_

- [x] 11. Wake Handlerの実装
  - [x] 11.1 Wake Handlerクラスの実装
    - WakeStateデータ構造を実装
    - WUPパターン認識ステートマシンを実装
    - WUPフィルタリング（tWK_FILTER）を実装
    - WUPタイムアウト（tWK_TIMEOUT）を実装
    - ローカルウェイクアップ（LWU）検出を実装
    - WAKERQフラグ管理を実装
    - WAKESRフラグ管理を実装
    - _Requirements: 6.1, 6.2, 6.3, 6.4, 6.5, 6.6_
  
  - [x] 11.2 Wake Handler用プロパティテストの作成
    - **Property 14: Wake-up pattern recognition**
    - **Validates: Requirements 6.1, 6.5**
  
  - [x] 11.3 Wake Handler用プロパティテストの作成
    - **Property 15: Local wake-up detection**
    - **Validates: Requirements 6.2**
  
  - [x] 11.4 Wake Handler用プロパティテストの作成
    - **Property 16: Wake-up flag indication**
    - **Validates: Requirements 6.3**
  
  - [x] 11.5 Wake Handler用プロパティテストの作成
    - **Property 17: Wake-up flag clearing**
    - **Validates: Requirements 6.4**
  
  - [x] 11.6 Wake Handler用単体テストの作成
    - WUPタイムアウトのエッジケースをテスト
    - _Requirements: 6.6_

- [x] 12. INH制御の実装
  - [x] 12.1 INH制御ロジックの実装
    - INH出力制御を実装（モードに基づく）
    - INH_MASK機能を実装
    - INHタイミング制御（tINH_SLP_STB）を実装
    - _Requirements: 7.1, 7.2, 7.3, 7.4, 7.5_
  
  - [x] 12.2 INH制御用プロパティテストの作成
    - **Property 18: INH pin control**
    - **Validates: Requirements 7.1, 7.2, 7.3, 7.4**
  
  - [x] 12.3 INH制御用単体テストの作成
    - INHタイミングのエッジケースをテスト
    - _Requirements: 7.5_

- [x] 13. Checkpoint - フォールトとウェイクアップ機能の検証
  - 全てのテストが成功することを確認
  - ユーザーに質問があれば確認

- [x] 14. Main Simulatorの統合
  - [x] 14.1 TCAN1463Q1Simulatorクラスの実装
    - メインシミュレータクラスを実装
    - 全コンポーネントを統合
    - 統一I/Oアクセスインターフェースを実装
    - バッチI/O操作を実装
    - ピンメタデータ取得を実装
    - シミュレーションステップメソッドを実装
    - 条件付き実行メソッド（run_until）を実装
    - _Requirements: 11.1, 11.2, 11.3, 11.4, 11.5, 11.6_
  
  - [x] 14.1.1 シミュレーションステップのバス同期修正
    - simulator_step()関数内のバス駆動と読み取りの順序を修正
    - バス駆動 → バス読み取り → RXD更新の順序に変更
    - TXDからRXDへの伝搬遅延が正しく動作することを確認
    - _Requirements: 9.1, 9.2_
  
  - [x] 14.2 Main Simulator用プロパティテストの作成
    - **Property 22: Propagation delay bounds**
    - **Validates: Requirements 9.1, 9.2**
  
  - [x] 14.3 Main Simulator用プロパティテストの作成
    - **Property 23: Flag clearing on mode transition to Normal**
    - **Validates: Requirements 10.9**

- [x] 15. 設定とパラメータ管理の実装
  - [x] 15.1 設定APIの実装
    - パラメータ設定メソッドを実装
    - パラメータ検証ロジックを実装
    - スナップショット/リストア機能を実装
    - _Requirements: 12.1, 12.2, 12.3, 12.4, 12.5, 12.6_
  
  - [x] 15.2 設定管理用プロパティテストの作成
    - **Property 25: Parameter validation**
    - **Validates: Requirements 12.6**
  
  - [x] 15.3 設定管理用単体テストの作成
    - スナップショット/リストア機能をテスト
    - _Requirements: 12.1-12.5_

- [x] 16. イベントコールバックシステムの実装
  - [x] 16.1 イベントシステムの実装
    - イベント型定義を実装
    - イベントコールバック登録/解除を実装
    - イベント発火ロジックを各コンポーネントに統合
    - _Requirements: 11.1, 11.2, 11.3, 11.4, 11.5_
  
  - [x] 16.2 イベントシステム用単体テストの作成
    - コールバック登録/解除をテスト
    - イベント発火をテスト
    - _Requirements: 11.1-11.5_

- [x] 17. Checkpoint - 統合機能の検証
  - 全てのテストが成功することを確認
  - ユーザーに質問があれば確認

- [x] 18. C言語インターフェースの実装
  - [x] 18.1 C APIヘッダーの作成
    - tcan1463q1_c_api.hを作成
    - extern "C"ブロックを定義
    - 不透明ハンドル型を定義
    - エラーコード列挙型を定義
    - C互換関数シグネチャを定義
    - _Requirements: 13.1, 13.2, 13.3, 13.4, 13.5_
  
  - [x] 18.2 C APIラッパーの実装
    - C++実装をラップするC互換関数を実装
    - 例外をエラーコードに変換
    - ハンドル管理を実装
    - エラーメッセージ文字列を実装
    - _Requirements: 13.1, 13.2, 13.3, 13.4, 13.7_
  
  - [x] 18.3 C API用単体テストの作成
    - Cコードからのシミュレーター使用をテスト
    - エラーハンドリングをテスト
    - ハンドル管理をテスト
    - _Requirements: 13.1-13.7_
  
  - [x] 18.4 C API使用例の作成
    - 基本的な使用例をCで作成
    - エラーハンドリング例を作成
    - _Requirements: 13.6_

- [~] 19. 統合テストの作成
  - [~] 19.1 モード遷移シナリオテストの作成
    - Normal → Standby → Sleep → Normal遷移をテスト
    - Silent modeの動作をテスト
    - Go-to-sleep modeの遷移をテスト
    - _Requirements: 2.1-2.8_
  
  - [~] 19.2 ウェイクアップシナリオテストの作成
    - Sleep → WUP受信 → Standby → Normalをテスト
    - Sleep → LWU → Standby → Normalをテスト
    - _Requirements: 6.1-6.6_
  
  - [~] 19.3 フォールト検出シナリオテストの作成
    - Normal → TXD timeout → Fault → Recoveryをテスト
    - 各種フォールト条件をテスト
    - _Requirements: 5.1-5.7_
  
  - [~] 19.4 電源シーケンステストの作成
    - Power-up → Normal operation → Undervoltage → Recoveryをテスト
    - _Requirements: 4.1-4.7_
  
  - [~] 19.5 CAN通信シナリオテストの作成
    - メッセージ送受信をテスト
    - バス状態遷移をテスト
    - _Requirements: 3.1-3.6_

- [x] 20. ドキュメントとサンプルコードの作成
  - [x] 20.1 APIドキュメントの作成
    - Doxygenコメントを全パブリックAPIに追加
    - README.mdを作成（ビルド手順、使用方法）
  
  - [x] 20.2 サンプルコードの作成
    - 基本的な使用例を作成（C++とC両方）
    - CAN通信シミュレーション例を作成
    - フォールト検出例を作成
    - ウェイクアップ例を作成

- [x] 21. 最終チェックポイント
  - 全てのテストが成功することを確認
  - コードカバレッジを確認
  - ドキュメントの完全性を確認
  - C/C++両方のAPIが正しく動作することを確認
  - ユーザーに最終確認

## Notes

- タスクに`*`が付いているものはオプションで、より速いMVPのためにスキップ可能です
- 各タスクは特定の要件を参照しており、トレーサビリティを確保しています
- チェックポイントは段階的な検証を保証します
- プロパティテストは普遍的な正確性プロパティを検証します
- 単体テストは特定の例とエッジケースを検証します
