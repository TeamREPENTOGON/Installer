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

window = tk.Tk()

textbox = tk.Text()
textbox.bind("<Key>", lambda e: "break")
button = tk.Button(text="Install")


def perform_install():
    global textbox
    global button

    textbox.insert("end", "Fetching latest release from GitHub...\\n")
    try:
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

        textbox.insert("end", "Fetching hash...\\n")

        with requests.get(hash_url) as r:
            hash = r.text.lower().rstrip()

        hash = r.text.lower().rstrip()

        textbox.insert("end", "Downloading...\\n")
        with requests.get(zip_url, stream=True) as r:
            zip = io.BytesIO(r.content)

        textbox.insert("end", "Hashing...\\n")
        calculated_hash = hashlib.sha256(zip.getbuffer()).hexdigest()

        if hash != calculated_hash:
            raise Exception(
                f"Invalid hash, download is corrupt!\\nExpected hash: {hash}\\nCalculated hash: {calculated_hash}"
            )

        textbox.insert("end", "Extracting...\\n")

        with zipfile.ZipFile(zip, "r") as zip_ref:
            zip_ref.extractall()

        textbox.insert("end", "Finished!")

        if len(sys.argv) > 1 and "-auto" in sys.argv:
            new_args = sys.argv
            new_args.remove("-auto")
            new_args.insert(0, "isaac-ng.exe")
            print(" ".join(new_args))
            subprocess.Popen(new_args, start_new_session=True)
            sys.exit()

    except Exception as e:
        print(e)
        textbox.insert("end", "Failed to install!\\n" + str(e) + "\\n")
    button["state"] = "normal"
    button["text"] = "Install"


def install():
    global button
    button["state"] = "disabled"
    button["text"] = "Installing..."
    t = threading.Thread(target=perform_install, daemon=True)
    t.start()


window.title("REPENTOGON Installer")

tk.Label(text="Welcome to the REPENTOGON Installer!").pack()
button["command"] = install
button.pack()
textbox.pack()

if len(sys.argv) > 1 and "-auto" in sys.argv:
    t = threading.Thread(target=perform_install, daemon=True)
    t.start()

window.mainloop()
