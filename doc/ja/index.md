% ヘルプインデックス

# 序章

[ドラムスティックMIDIモニター](https://kmidimon.sourceforge.io) は今後のイベントを記録します 
MIDI外部ポートまたはアプリケーションから
ALSAシーケンサーを介して、またはスタンダードMIDIファイルとして保存されます。です
MIDIソフトウェアまたはMIDIセットアップをデバッグする場合に特に便利です。
優れたグラフィカルユーザーインターフェイス、カスタマイズ可能なイベントフィルターを備えています
シーケンサーパラメーター、すべてのMIDIメッセージと一部のALSAのサポート
メッセージ、および記録されたイベントリストをテキストファイルまたはSMFに保存します。

# 入門

## メインウィンドウ

プログラムは録音状態で開始し、すべての着信MIDIを登録します
停止ボタンを押すまでイベント。再生するボタンもあります、
他のメディアの通常の動作で、一時停止、巻き戻し、早送り
プレーヤー。

イベントリストグリッドの上に、それぞれに1つずつタブのセットがあります。
SMFで定義されたトラック。新しいタブを追加したり、タブを閉じることができます。
記録されたイベントはビューまたはイベントのみであるため、失われます
フィルタ。

プログラムへのALSAシーケンサーのMIDI接続を制御できます。
DrumstickMIDIモニター内のデバイス。これを行うには、メニューの下のオプションを使用します
メインメニューの「接続」。接続するオプションがあります
ドラムスティックMIDIモニターへの使用可能なすべての入力ポートとダイアログボックスを切断します
ここで、監視するポートと出力ポートを選択できます。

次のようなMIDI接続ツールを使用することもできます
[aconnect（1)](https://linux.die.net/man/1/aconnect)
または[QJackCtl](https://qjackctl.sourceforge.io) でアプリケーションに接続します
またはドラムスティックMIDIモニターへのMIDIポート。

MIDIOUTポートがDrumstickMIDIMonitorの入力ポートに接続されている場合
録音状態、すべてがそうである場合、それは着信MIDIイベントを表示します
正しい。

受信した各MIDIイベントは1行に表示されます。列には
次の意味。

* **ティック**：イベント到着の音楽時間
* **時間**：イベント到着の秒単位のリアルタイム
* **ソース**：発信元のMIDIデバイスのALSA識別子
    イベント。どのイベントがどのイベントに属しているかを認識できるようになります
    複数を同時に接続している場合はデバイス。あります
    の代わりにALSAクライアント名を表示する構成オプション
    番号
* **イベントの種類**：イベントタイプ：メモのオン/オフ、コントロールの変更、ALSA、および
    すぐ
* ** Chan **イベントのMIDIチャンネル（チャンネルイベントの場合）。それ
    Sysexチャンネルを表示するためにも使用されます。
***データ1 **：受信したイベントのタイプによって異なります。コントロールの場合
    イベントまたはノートを変更します。これは、管理番号またはノート番号です。
***データ2 **：受信したイベントのタイプによって異なります。コントロールの場合
    変更すると値になり、Noteイベントの場合は
    速度
***データ3 **：システム排他的またはメタイベントのテキスト表現。

コンテキストメニューを使用して、任意の列を表示または非表示にすることができます。これを開くには
メニューで、イベントリスト上でマウスの2番目のボタンを押します。あなたもすることができます
[構成]ダイアログを使用して、表示されている列を選択します。

イベントリストには、常に、記録された新しいイベントが下部に表示されます。
グリッド。

Drumstick MIDI Monitorは、記録されたイベントをテキストファイル（CSV形式）または
スタンダードMIDIファイル（SMF）。

## 構成オプション 

[構成]ダイアログを開くには、[設定]→[構成]メニューオプションに移動します
または、ツールバーの対応するアイコンをクリックします。

これは、構成オプションのリストです。

***シーケンサータブ**。キューのデフォルト設定はイベントの時間に影響します
    精度。
*** [フィルター]タブ**。ここでは、いくつかのイベントファミリーを確認できます
    イベントリストに表示されます。
***タブを表示**。チェックボックスの最初のグループでは、
    イベントリストの対応する列。
***その他 タブ**。その他のオプションは次のとおりです。
    + ALSAクライアントIDを名前に変換します。チェックした場合、ALSAクライアント
      「ソース」列のID番号の代わりに名前が使用されます
      イベントのすべての王、およびALSAイベントのデータ列。
    +ユニバーサルシステムエクスクルーシブメッセージを翻訳します。チェックした場合、
      ユニバーサルシステムエクスクルーシブメッセージ（リアルタイム、非リアルタイム、
      MMC、MTC、およびその他のいくつかのタイプ）が解釈され、翻訳されます。
      それ以外の場合は、16進ダンプが表示されます。
    +固定フォントを使用します。デフォルトでは、Drumstick MIDIMonitorはシステムフォントを使用します
      イベントリスト。このオプションをオンにすると、固定フォントが使用されます
      代わりは。

# クレジットとライセンス

プログラムCopyright©2005-2021Pedro Lopez-Cabanillas

ドキュメントCopyright©2005Christoph Eckert

ドキュメントCopyright©2008-2021Pedro Lopez-Cabanillas

# インストール

## ドラムスティックMIDIモニターの入手方法 

[ここ](https://sourceforge.net/projects/kmidimon/files/)
あなたは最後のバージョンを見つけることができます。にはGitミラーもあります
[GitHub](https://github.com/pedrolcl/kmidimon)

## 要件

Drumstick MIDI Monitorを正常に使用するには、Qt 5、Drumstick2が必要です。
およびALSAドライバーとライブラリ。

ビルドシステムには[CMake](http://www.cmake.org) 3.14以降が必要です。

ALSAライブラリ、ドライバ、およびユーティリティは、次の場所にあります。
[ALSAホームページ](http://www.alsa-project.org)

変更点のリストは https://sourceforge.net/p/kmidimon にあります。

## コンパイルとインストール

Drumstick MIDI Monitorをコンパイルしてシステムにインストールするには、次のように入力します。
Drumstick MIDIMonitorディストリビューションのベースディレクトリで次のようになります。

    $ cmake .
    $ make
    $ make install

ドラムスティックMIDIモニターは `cmake` と `make` を使用しているので問題ありません
それをコンパイルします。問題が発生した場合は、に報告してください
著者またはプロジェクトのバグ追跡システム。