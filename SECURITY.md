# Security policy

## Supported release

Security reports are accepted for the latest tagged release on its documented
platform: Windows XP Professional x64 Edition SP2.

## Reporting

Use GitHub's **Report a vulnerability** / private Security Advisory workflow.
Do not publish exploit details, credentials, personal data, or Windows system
binaries in a normal issue.

## Expected high-risk behavior

eXPerience2K is a protected-system-file modifier. It requests administrator
rights, creates recoverable backups, modifies PE resources, and temporarily
suspends Windows File Protection so requested files can be replaced. That
suspension path enables `SeDebugPrivilege`, interacts with Winlogon, and starts
the XP system WFP suspension routine in the target process. Antivirus products
may flag these techniques even when no unrelated malicious payload is present.

The installer and application are unsigned. Users should verify the SHA-256
published in the release notes and keep a complete recovery path.

## Behavior that is not expected

The application contains no network code and should not:

- access credentials, product keys, or personal documents;
- transmit telemetry or download payloads;
- persist outside the documented startup reloader;
- change files or settings outside its documented feature scope;
- conceal its installed components; or
- delete recovery data after a failed restoration.

Reports involving any of those behaviors should be treated as security issues.

## Diagnostics privacy

The configuration log remains in memory unless the user explicitly opens or
saves it. Sanitized logs must not include account names, SIDs, profile paths,
product keys, or installation-media keys.
