## 日本語は下記です。

# Overview
C++ Maya vector displacement plugin. Supports displacement maps in object-space and tangent-space.

![Maya Vector Displacement Deformer](https://github.com/Zhibade/maya-vector-displacement-deformer/raw/master/docs/MayaVectorDisplacementShader.gif)


# How to use
- Compile the plugin
- Load the plugin in Maya (Window -> Settings/Preferences -> Plugin Manager -> Reference -> Plugin file (.mll, .bundle or .so))
- In any "Deform" menu click the *Vector Displacement* entry or run the following MEL command: `deformer -type vectorDisplacement;`
- In the attribute editor, in the *Vector Displacement Map* attribute load the vector displacement file.
- Set the *Displacement Map Type* attribute to match the type of the loaded vector displacement map.
  - Object = Object-space
  - Tangent = Tangent-space
- The *Strength* attribute controls how much to apply the effect.


# How to build
- Download the Maya SDK and extract it to any directory. The current version can be found here: [Maya Developer Network](https://www.autodesk.com/developer-network/platform-technologies/maya?_ga=2.264747919.1618081658.1597322765-1818450911.1593850237).
- In the *CMakeLists.txt* set the SDK folder location (line 10).
- Build the plugin according to your OS.



# 概要
C++のMayaのベクターディスプレイスメントデフォーマ。オブジェクト空間または接空間のディスプレイスメントマップ対応です。

![Maya Vector Displacement Deformer](https://github.com/Zhibade/maya-vector-displacement-deformer/raw/master/docs/MayaVectorDisplacementShader.gif)


# 使い方
- プラグインをコンパイルします。
- Mayaでプラグインをロードします。（ウィンドウ　→　設定／プリファレンス　→　プラグイン マネージャ　→　参考　→　プラグインファイル（「.mll」、「.bundle」または「.so」））
- デフォームメニューで「Vector Displacement」の項目をクリックします。あるいは次のMELコマンドを実行します：`deformer -type vectorDisplacement;`
- アトリビュートエディタで「Vector Displacement Map」のアトリビュートにディスプレイスメントファイルをロードします。
- ロードしたディスプレイスメントのタイプに合わせるように「Displacement Map　Type」のアトリビュートを設定します。
  - Object＝オブジェクト空間。
  - Tangent＝接空間。
- 「Strength」のアトリビュートでディスプレイスメントの強度を変更できます。


# ビルド方法
- MayaのSDKをダウンロードして、どこでもファイルを展開します。現在のバージョンはこちらです：[Maya Developer Network](https://www.autodesk.com/developer-network/platform-technologies/maya?_ga=2.264747919.1618081658.1597322765-1818450911.1593850237)
- 「CMakeLists.txt」でMayaのSDKフォルダを設定します。（行10)
- OSによって、プラグインをビルドします。