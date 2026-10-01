# Warcraft III 1.26a - NVIDIA App Profile / RTX HDR Fix

A lightweight 32-bit proxy DLL wrapper for **Warcraft III (v1.26a / Game.dll build 6401)** that fixes the game profile resetting behavior in the new **NVIDIA App** (NVIDIA Freestyle / RTX HDR).

## The Problem
Warcraft III 1.26a uses an aggressive internal security feature (`ProtectProcess`) inside `Game.dll` that tightens the process DACL at startup. This denies all local users and external software the right to query the process. As a result, the **NVIDIA App cannot read the `war3.exe` path**, causing game filters, custom profiles, and **RTX HDR** to reset to defaults upon every launch.

## What this Fix Does
This mod acts as a proxy wrapper for the system `version.dll`. 
1. It intercepts the game's earliest calls to file version structures right at boot.
2. It dynamically patches the memory inside `Game.dll` to bypass the restrictive DACL change, leaving the executable path readable to external software (specifically `PROCESS_QUERY_LIMITED_INFORMATION`).
3. It transparently forwards all standard calls to the legitimate Windows `C:\Windows\SysWOW64\version.dll` library.

## Required Companion: dgVoodoo 2
> [!IMPORTANT]
> Warcraft III 1.26a renders through **DirectX 8**, and the NVIDIA App cannot apply **RTX HDR** to a native DX8 swapchain. You must run the game through [**dgVoodoo 2**](http://dege.freeweb.hu/dgVoodoo2/dgVoodoo2/), which wraps the old DX8 output into a modern DirectX 11/12 device that the NVIDIA overlay can hook.

This fix and dgVoodoo 2 solve two different halves of the same problem and are meant to be used **together**:

* **dgVoodoo 2** translates DX8 → DX11/12 so that RTX HDR *can* be applied at all.
* **This `version.dll` fix** keeps the NVIDIA App from *resetting* your profile and RTX HDR settings on every launch.

Without dgVoodoo 2, RTX HDR will not work on Warcraft III regardless of this fix. Without this fix, your dgVoodoo + RTX HDR profile will reset each time you start the game.

---

## Important Anti-Cheat Warning
> [!WARNING]
> Since this modification hooks internal memory structures inside `Game.dll` at runtime, **third-party anti-cheat software or custom multiplayer platforms** (such as iCCup, Eurobattle, or local tournament clients) **may flag this DLL as a hack/modification** and ban your account. Use this fix at your own risk. It is highly recommended for offline/LAN use or single-player campaigns only.

---

## Antivirus False Positives
> [!NOTE]
> Some antivirus engines (notably Windows Defender) may flag `version.dll` with a heuristic name such as `Trojan:Win32/Wacatac.C!ml`. The `!ml` suffix means the detection comes from a **machine-learning heuristic**, not a signature match against a known threat. `Wacatac` is a generic catch-all label.

This is a **false positive**. The fix is flagged because its legitimate technique shares a shape with code-injection malware:

* **It is named `version.dll` and placed next to the game.** Loading a DLL that shadows a system library name is a well-known pattern used by both mods and malware, and heuristics cannot tell them apart.
* **It patches memory inside `Game.dll`.** To bypass the `ProtectProcess` DACL change, it makes a single `call` instruction writable, rewrites it to point at the fix, and flushes the instruction cache. This "find address → make writable → patch bytes" sequence is what the ML model reads as injection.
* **It modifies its own process DACL.** Re-opening the executable path to `PROCESS_QUERY_LIMITED_INFORMATION` requires setting an access-control list, which heuristics associate with persistence or hiding.

All three behaviours are the fix itself and apply **only to Warcraft III's own process**. The DLL makes no network connections, writes nothing to disk, and does not persist. You can verify this by auditing `src/version.c` and compiling it yourself (see below).

**If you want to use the pre-compiled binary:**
1. Audit the source or build it yourself from `src/`.
2. Add your Warcraft III folder to your antivirus exclusions.
3. Optionally, submit the file to [Microsoft's false-positive form](https://www.microsoft.com/en-us/wdsi/filesubmission) so the detection can be reviewed.

---

## Installation

1. Download the pre-compiled `version.dll` from the latest release.
2. Place the `version.dll` file directly into your **Warcraft III root folder** (the same folder where `war3.exe` and `Game.dll` are located).
3. **Do not rename or copy any files from your Windows folder.** The proxy handles system routing automatically.
4. Launch the game normally. Open your NVIDIA App overlay (`Alt + Z`) to configure and lock your RTX HDR settings.

---

## How to Compile Independently (DIY)

If you do not trust pre-compiled binaries, you can easily audit the source code (`version.c` and `version.def`) and cross-compile the 32-bit Proxy DLL yourself. 

Below is an example of compilation using **WSL (Windows Subsystem for Linux) with Ubuntu** environment.

### Prerequisites
Open your WSL Ubuntu terminal and install the MinGW-w64 cross-compiler toolset:
```bash
sudo apt update && sudo apt install -y mingw-w64
```

### Compilation Step
Navigate to the directory containing `version.c` and `version.def`, then run the following command to build a clean x86 PE binary:

```bash
i686-w64-mingw32-gcc -O2 -Wall -shared -o version.dll version.c version.def -lversion -Wl,--enable-stdcall-fixup
```

### Compiler Flags Explained:
* `-shared`: Instructs the linker to build a dynamic-link library (.dll).
* `i686-w64-mingw32-gcc`: Forces native 32-bit x86 generation (required for Warcraft III 1.26a).
* `version.def`: Feeds the export definitions file directly into the linker to avoid recursion bugs.
* `-Wl,--enable-stdcall-fixup`: Resolves Windows `__stdcall` decorations for clean 32-bit function exports without warnings.

Once compiled, grab your newly generated `version.dll` and move it to your game folder.

## License
W3 RTX HDR Fix is a mod for Warcraft III 1.26a, MIT licensed, see `LICENSE`. Author: [Hr0ffT](https://github.com/Hr0ffT).
