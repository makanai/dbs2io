# dbs2io

指定されたプロセスを起動し、OutputDebugStringの出力をフックして標準出力へ出力する。

Usege: dbs2io.exe [-exec:execFilename] [-work:currentDirectory] [-exit:exitOnText] [-time:timeOutSec]
 -exec: 実行ファイル名
 -work: 実行ファイルのワーキングディレクトリ
 -exit: 終了文字列。これが標準出力に出るとdbs2ioは正常終了する。
 -time: タイムアウト時間。この時間以上の標準出力が無い場合、dbs2ioはエラー終了する。

 使い方としては。
 1. デバッグ出力で動作するアプリケーションを、dbs2io経由で起動する。デバッグ出力が標準出力に流れるようになる。
 2. テスト組む。正常終了時にキーワードを出力するようにしとく。仮に"success"とする。
 3. dbs2ioで-exitでキーワード"success"を指定すると、dbs2io経由で正常終了を検出できるようになる。
 4. dbs2ioで-timeで時間指定すると、何らかの理由でテストが終わらない場合、エラーを検出できるようになる。
 という感じ。
 某ps3ctr, psp2ctrあたりを参考にWin32/Win64で互換動作するツール、という位置づけ。
