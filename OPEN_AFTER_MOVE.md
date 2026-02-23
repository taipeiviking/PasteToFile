# After Moving To `C:\git\PasteToFile`

This project was originally worked on from a Google Drive mapped path (`H:\My Drive\...`).
That filesystem can cause permission/locking issues with git operations.

After copying the entire folder (including `.git`) to `C:\git\PasteToFile`, follow these steps.

## 1) Open the repo from the new location

- Open Cursor / VS Code on: `C:\git\PasteToFile`

## 2) Sanity check git state

From `C:\git\PasteToFile`:

```powershell
git status -b
git log --oneline --decorate -5
git remote -v
git branch -vv
git fetch origin
git log --oneline --decorate --graph --all --max-count 20
```

Repo on GitHub:

- https://github.com/taipeiviking/PasteToFile

## 3) Push to GitHub

If the remote repo already has an auto-created initial commit, git may reject your push as
non-fast-forward. You have two choices:

### Option A (recommended): overwrite remote `main`

Use this if you do not care about preserving the remote “initial commit”:

```powershell
git push --force-with-lease -u origin main
```

### Option B: preserve remote history (merge unrelated histories)

Use this if you want to keep the remote initial commit:

```powershell
git merge origin/main --allow-unrelated-histories
git push -u origin main
```

## 4) Build the installer

The MSI is built from:

- `installer/PasteToFileInstaller.wixproj`

Build command (example):

```powershell
& "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\MSBuild\Current\Bin\MSBuild.exe" `
  "installer\PasteToFileInstaller.wixproj" /restore /t:Build /p:Configuration=Release /p:Platform=x64
```

Output:

- `installer\bin\x64\Release\en-us\PasteToFileInstaller.msi`

Optional setup EXE bundle (chains VC++ redist; may trigger UAC/admin prompt):

- `installer\bin\x64\Release\PasteToFileSetup.exe`

## 5) Quick runtime notes

- Windows 11: the menu appears under `Show more options` (classic context menu).
- Logs:
  - `%LOCALAPPDATA%\PasteToFile\ptf.log`
  - `ptf-debug.log` (see `README.md` for the search order and `PTF_LOG_DIR`)

