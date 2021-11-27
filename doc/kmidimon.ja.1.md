% KMIDIMON(1) kmidimon 0.0.0 | Drumstick MIDI Monitor
% Pedro López-Cabanillas <plcl@users.sf.net>

# NAME

**kmidimon** - ALSA シーケンサと KDE ユーザインタフェースを使うMIDIモニタ

# SYNOPSIS

| **kmidimon** \[ options ] \[ file ]

# 説明

KMidimonは、MIDI外部ポートかALSAシーケンサ経由でアプリケーションから来るイベントをモニタするか、それを標準MIDIファイルとして格納します。これはMIDIソフトウェアのデバッグか、MIDIのセットアップを行いたい場合、特に便利です。これには快適なグラフィカルユーザインタフェースがあり、カスタマイズ可能なイベントフィルタとシーケンサパラメータがあり、すべてのMIDIメッセージといくつかのALSAメッセージをサポートし、テキストファイルまたはSMFとして記録されたイベントリストを保存します。

# オプション

## 般的なオプション:

`--help`

:   オプションについてのヘルプを表示します。

`--help-all`

:   すべてのオプションを表示します。

`-v`,`--version`

:   バージョン情報を表示します。

## QT オプション:

`--style` `style`

:   アプリケーションの GUI スタイルを設定します。

`--geometry` `geometry`

:   メインウイジットのクライアントの位置を設定します。

`--display` `displayname`

:   X サーバのdisplayとして `displayname`を使います。

`--session` `sessionId`

:   与えられた `sessionId`でアプリケーションをリストアします。

`--nograb`

:   QT に、マウスかキーボードイベントを決して取らないようにさせます。

`--dograb`

:   デバッガは以下で動かしている時は暗黙で`--nograb`
    になるので、それを無視する時には`--dograb`を使います。

`--name` `name`

:   アプリケーション名を指定します。

`--title` `title`

:   アプリケーションタイトル(caption)を指定します。

`--reverse`

:   ウイジェットの全体レイアウトをミラーします。

`--stylesheet` file.qss

:   アプリケーションウイジェットに QT スタイルシートを適用します。

## Arguments

`file`
:   Input SMF/RMI/KAR/WRK file name.

# BUGS

See Tickets at Sourceforge <https://sourceforge.net/p/kmidimon/>

# SEE ALSO

**qt5options (7)**

# 著作権

このマニュアルは Pedro Lopez-Cabanillas <plcl@users.sourceforge.net>
によって書かれました。 Permission is granted to copy, distribute and/or
modify this document under the terms of the GNU General Public License,
Version 2 or any later version published by the Free Software
Foundation; considering as source code all the file that enable the
production of this manpage.
