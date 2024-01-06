import tkinter as tk
import requests
import json
import threading
import shutil
import hashlib
import zipfile
import io
import subprocess
import sys
import os

window = tk.Tk()

textbox = tk.Text()
textbox.bind("<Key>", lambda e: "break")
install_button = tk.Button(text="Install")
uninstall_button = tk.Button(text="Uninstall")


def perform_install():
    global textbox
    global install_button
    global uninstall_button

    try:
        if not os.path.isfile("isaac-ng.exe"):
            raise Exception("Installation must be performed from the game's directory!")

        textbox.insert("end", "Fetching latest release from GitHub...\n")
        with requests.get(
            "https://api.github.com/repos/TeamREPENTOGON/REPENTOGON/releases/latest",
            headers={
                "User-Agent": "REPENTOGON",
                "Accept": "application/vnd.github+json",
                "X-Github-Api-Version": "2022-11-28",
            },
        ) as r:
            doc = json.loads(r.text)

        hash_url = ""
        zip_url = ""

        for asset in doc["assets"]:
            if asset["name"] == "hash.txt":
                hash_url = asset["browser_download_url"]
            if asset["name"] == "REPENTOGON.zip":
                zip_url = asset["browser_download_url"]

        textbox.insert("end", "Fetching hash...\n")

        with requests.get(hash_url) as r:
            hash = r.text.lower().rstrip()

        hash = r.text.lower().rstrip()

        textbox.insert("end", "Downloading...\n")
        with requests.get(zip_url, stream=True) as r:
            zip = io.BytesIO(r.content)

        textbox.insert("end", "Hashing...\n")
        calculated_hash = hashlib.sha256(zip.getbuffer()).hexdigest()

        if hash != calculated_hash:
            raise Exception(
                f"Invalid hash, download is corrupt!\nExpected hash: {hash}\nCalculated hash: {calculated_hash}"
            )

        textbox.insert("end", "Extracting...\n")

        with zipfile.ZipFile(zip, "r") as zip_ref:
            zip_ref.extractall()

        textbox.insert("end", "Finished!")

        if len(sys.argv) > 1 and "-auto" in sys.argv:
            new_args = sys.argv
            new_args.remove("-auto")
            new_args.insert(0, "isaac-ng.exe")
            subprocess.Popen(new_args, start_new_session=True)
            os._exit(0)

    except Exception as e:
        textbox.insert("end", "Failed to install!\n" + str(e) + "\n")

    install_button["state"] = "normal"
    uninstall_button["state"] = "normal"
    install_button["text"] = "Install"


def perform_uninstall():
    global install_button
    global uninstall_button
    global textbox

    try:
        files = [
            "dsound.dll",
            "freetype.dll",
            "libzhl.dll",
            "Lua5.4.dll",
            "zhlREPENTOGON.dll",
            "dsound.log",
            "repentogon.log",
            "zhl.log",
            "resources/scripts/main_ex.lua",
            "resources/scripts/enums_ex.lua",
            "resources/scripts/pysireffer.py",  # we really don't need to ship this anymore, do we?
            "resources/scripts/repentogon_extras",
            "resources/scripts/repentogon_tests",
            "resources/rooms/26.The Void_ex.stb",
            "resources/shaders/coloroffset_gold_mesafix.fs",
            "resources/shaders/coloroffset_gold_mesafix.vs",
            "resources-repentogon",
        ]

        cleanup_dirs = ["resources/rooms", "resources/shaders"]

        if not os.path.isfile("isaac-ng.exe"):
            raise Exception(
                "Uninstallation must be performed from the game's directory!"
            )
            return

        for file in files:
            try:
                if os.path.isfile(file):
                    os.remove(file)
                    textbox.insert("end", f"Successfully removed {file}\n")
                elif os.path.isdir(file):
                    shutil.rmtree(file)
                    textbox.insert("end", f"Successfully removed directory {file}\n")

            except FileNotFoundError:
                pass

        for directory in cleanup_dirs:
            if os.path.isdir(directory) and not os.listdir(directory):
                shutil.rmtree(directory)
                textbox.insert("end", f"{directory} empty, removing\n")

        textbox.insert("end", "Finished!")

    except Exception as e:
        textbox.insert("end", "Failed to uninstall!\n" + str(e) + "\n")

    install_button["state"] = "normal"
    uninstall_button["state"] = "normal"
    uninstall_button["text"] = "Uninstall"


def install():
    global install_button
    global uninstall_button
    global textbox

    install_button["state"] = "disabled"
    uninstall_button["state"] = "disabled"
    install_button["text"] = "Installing..."
    textbox.delete(1.0, tk.END)
    t = threading.Thread(target=perform_install, daemon=True)
    t.start()


def uninstall():
    global install_button
    global uninstall_button
    global textbox

    install_button["state"] = "disabled"
    uninstall_button["state"] = "disabled"
    uninstall_button["text"] = "Uninstalling..."
    textbox.delete(1.0, tk.END)

    t = threading.Thread(target=perform_uninstall, daemon=True)
    t.start()


window.title("REPENTOGON Installer")

tk.Label(text="Welcome to the REPENTOGON Installer!").pack()

install_button["command"] = install
install_button.pack()

uninstall_button["command"] = uninstall
uninstall_button.pack()

textbox.pack()

if len(sys.argv) > 1 and "-auto" in sys.argv:
    t = threading.Thread(target=perform_install, daemon=True)
    t.start()

window.mainloop()
