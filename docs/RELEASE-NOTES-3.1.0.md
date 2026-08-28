Version 3.1.0 adds configurable backgrounds behind the classic logon prompt.

### What's new

- Choose the existing blue (`#3A6EA5`), Windows 95 teal (`#008080`), or your own image.
- PNG, JPG/JPEG, and BMP files are automatically converted to an XP-compatible
  24-bit BMP. The source is untouched, proportions are preserved, and
  transparent areas/unused space are filled with blue.
- The converted image is saved locally in the application's Windows folder,
  so it works before sign-in and does not depend on the original file remaining
  in your account or on removable media.
- The choice is remembered. Existing 3.0.0 restore baselines are retained;
  clearing classic logon, Revert, and uninstall restore the original settings.
- The signed-in wallpaper, branding, and independent title-bar presets are
  unchanged. The initial window now also fits smaller desktop work areas.

Enable **Windows 2000 style login window**, choose a **Logon background**,
click **Apply**, and restart. For custom images, choose **Custom image** and
then **Choose image...**. Images may be up to 64 MB and 32 megapixels.

Still supports only English **Windows XP Professional x86 SP3** and
**Windows XP Professional x64 Edition SP2**. All eleven existing features
remain available. No kernel/boot-screen changes are included.

### Download

The only attached binary is `eXPerience2K-v3.1.0-Setup.exe`.

```text
SHA-256: 68314B8A8AF8F47772E4C4B239708784D6B8E95B6CC03ADA327B30ECB73FC9F8
Size: 4342170 bytes
```

The installer is unsigned. Keep a system backup before modifying protected
Windows resources. See the repository's README and validation record for
usage, restoration, test coverage, and limitations.
