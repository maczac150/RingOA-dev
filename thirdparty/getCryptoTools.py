import os
import sys
import platform


def getCryptoTools(install, prefix, par, cryptoTools, boost, relic):
    # 現在のディレクトリを記録
    cwd = os.getcwd()

    # cryptoToolsの取得
    if not os.path.isdir("cryptoTools"):
        os.system("git clone https://github.com/ladnir/cryptoTools.git")

    # cryptoToolsディレクトリに移動してブランチを固定
    os.chdir(os.path.join(cwd, "cryptoTools"))
    os.system("git checkout d29f143a86ca71490201c52df9b6d813b618cda8 --quiet")
    os.system("git submodule update --init --recursive")

    # OSごとの設定
    osStr = platform.system()
    debug = "--debug" if "--debug" in sys.argv else ""
    sudo = "--sudo" if install and osStr != "Windows" and "--sudo" in sys.argv else ""

    # インストール先ディレクトリ
    if not install:
        prefix = os.path.join(cwd, "win" if osStr == "Windows" else "unix")
        if debug:
            prefix += "-debug"

    # コマンド生成用
    installCmd = f"--install={prefix}" if prefix else (
        "--install" if install else "")
    cmakePrefix = f"-DCMAKE_PREFIX_PATH={prefix}" if prefix else ""

    # 実行コマンドを生成
    baseCmd = f"python3 build.py {sudo} --par={par} {installCmd} {debug}"
    boostCmd = f"{baseCmd} --setup --boost"
    relicCmd = f"{baseCmd} --setup --relic"
    cryptoToolsCmd = f"{baseCmd} -D ENABLE_RELIC={'ON' if relic else 'OFF'} -D ENABLE_BOOST=ON {cmakePrefix}"

    # コマンド出力
    print("\n\n=========== getCryptoTools.py ================")
    if boost:
        print("Boost setup command:", boostCmd)
    if relic:
        print("Relic setup command:", relicCmd)
    if cryptoTools:
        print("CryptoTools setup command:", cryptoToolsCmd)
    print("vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv\n\n")

    # コマンド実行
    if boost:
        os.system(boostCmd)
    if relic:
        os.system(relicCmd)
    if cryptoTools:
        os.system(cryptoToolsCmd)

    # 元のディレクトリに戻る
    os.chdir(cwd)


if __name__ == "__main__":
    # デフォルト値で実行
    getCryptoTools(False, "", 1, True, True, False)
