// =====================================================================
// KeyNest - SysML v2 textual model (OMG SysML v2 textual notation)
// Course project, discipline "ZhTsRPO" / "ЖЦРПО", lab work #3.
// File: srs/sysml/keynest-model.sysml
//
// Content: semantic model - system context, internal structure, use cases,
// action signatures, business requirements BR-01..BR-10 and a representative
// set of derived system requirements REQ-* (the complete set is in the SRS).
// Graphical views and GUI mock-ups are declared in keynest-views.sysml and
// rendered to ../img/*.png by tools/render_sysml.py.
//
// NOTE: names of people / organisations in doc comments are fictional
// training personas; replace them with the real customer / team mate data.
// =====================================================================

package KeyNestModel {
    private import ScalarValues::*;

    doc /* KeyNest is a new, offline-first DESKTOP password manager for a
           single user and a single device. It stores credentials in one
           locally encrypted vault file and never requires a cloud account. */;

    // -----------------------------------------------------------------
    // 1. Context: actors and external systems (sysml part definitions)
    // -----------------------------------------------------------------
    part def VaultOwner {
        doc /* The person who owns the vault. Customer persona: Elena Sokolova,
               office manager of a 12-person company. */;
    }
    part def TargetApplication {
        doc /* Any desktop application or web page with a login form that
               receives credentials through autofill. */;
    }
    part def OperatingSystemClock {
        doc /* Local OS time source, optionally disciplined by NTP. Needed
               to compute RFC 6238 TOTP codes. */;
    }
    part def OSKeychain {
        doc /* Windows Credential Manager / macOS Keychain / libsecret.
               Stores the wrapped vault key, never the master password. */;
    }
    part def EncryptedVaultFile {
        doc /* Single file vault.knx: AES-256-GCM encrypted records plus a
               clear-text header with version, salt and Argon2id parameters. */;
    }
    part def PwnedPasswordsService {
        doc /* Optional external service (Have I Been Pwned, k-anonymity API).
               Only the first 5 characters of a SHA-1 hash leave the device. */;
    }
    part def UpdateService {
        doc /* Optional external service (GitHub Releases) used only for an
               update notification. Disabled by default. */;
    }

    // -----------------------------------------------------------------
    // 2. System structure
    // -----------------------------------------------------------------
    part def KeyNestApp {
        part ui               : UserInterface;
        part vaultService     : VaultService;
        part crypto           : CryptoService;
        part generator        : PasswordGenerator;
        part autofill         : AutofillController;
        part categoryService  : CategoryService;
        part noteService      : NoteService;
        part totpService      : TotpService;
        part audit            : SecurityAudit;
        part storage          : EncryptedStorage;
        part backup           : BackupService;
        part importExport     : ImportExportService;
    }
    part def UserInterface;
    part def VaultService;
    part def CryptoService;
    part def PasswordGenerator;
    part def AutofillController;
    part def CategoryService;
    part def NoteService;
    part def TotpService;
    part def SecurityAudit;
    part def EncryptedStorage;
    part def BackupService;
    part def ImportExportService;

    part def VaultRecord {
        attribute title       : String;
        attribute username    : String;
        attribute password    : String;   // encrypted at rest
        attribute url         : String;
        attribute category    : String;
        attribute note        : String;
        attribute totpSecret  : String;
        attribute modifiedAt  : String;
    }

    // -----------------------------------------------------------------
    // 3. Behaviour: action signatures (black-box level, not an algorithm)
    // -----------------------------------------------------------------
    action def UnlockVault {
        in  masterPassword : String;
        in  totpCode       : String;
        in  recoveryKey    : String;
        out unlocked       : Boolean;
        out attemptsLeft   : Integer;
    }
    action def GeneratePassword {
        in  length          : Integer;
        in  upperLowerDigit : Boolean;
        in  symbols         : Boolean;
        in  excludeAmbiguous: Boolean;
        out password        : String;
        out entropyBits     : Real;
    }
    action def SaveRecord {
        in  record   : VaultRecord;
        out recordId : String;
    }
    action def AutofillCredentials {
        in  windowTitle : String;
        in  domain      : String;
        out recordId    : String;
        out username    : String;
        out password    : String;
    }
    action def ComputeTotp {
        in  secret : String;
        in  time   : Integer;
        out code   : String;
        out secondsLeft : Integer;
    }
    action def ExportVault {
        in  masterPassword : String;
        in  format         : String;
        out file           : String;
    }

    // -----------------------------------------------------------------
    // 4. Use cases (top level, mapped to business requirements in doc)
    // -----------------------------------------------------------------
    use case def KeyNestUseCases {
        use case 'UC-01 Unlock vault with 2FA'      { doc /* BR-01, BR-07 */; }
        use case 'UC-02 Generate strong password'   { doc /* BR-02 */; }
        use case 'UC-03 Create or edit a record'    { doc /* BR-03, BR-06 */; }
        use case 'UC-04 Organise with categories'   { doc /* BR-05 */; }
        use case 'UC-05 Search and filter'          { doc /* BR-05 */; }
        use case 'UC-06 Autofill credentials'       { doc /* BR-04, includes UC-01 */; }
        use case 'UC-07 Show TOTP code'             { doc /* BR-07 */; }
        use case 'UC-08 Run security audit'         { doc /* BR-09, optionally HIBP */; }
        use case 'UC-09 Backup import and export'   { doc /* BR-08 */; }
        use case 'UC-10 Work offline'               { doc /* BR-10 */; }
    }

    // -----------------------------------------------------------------
    // 5. Business requirements from the Customer (structured metadata)
    // -----------------------------------------------------------------
    metadata def BusinessRequirementTag {
        attribute id        : ScalarValues::String;
        attribute priority  : ScalarValues::String;
        attribute source    : ScalarValues::String;
        attribute statement : ScalarValues::String;
        attribute release   : ScalarValues::String;
    }
    requirement def BusinessRequirement;

    @BusinessRequirementTag {
        id = "BR-01"; priority = "MUST"; source = "Customer"; release = "R1";
        statement = "One local vault must be opened with a master password and must stay encrypted at rest.";
    }
    requirement 'BR-01 Local encrypted vault' : BusinessRequirement {
        doc /* Acceptance: after 5 wrong passwords a 30 s delay is applied;
               no record is readable from vault.knx without the key. */;
    }

    @BusinessRequirementTag {
        id = "BR-02"; priority = "MUST"; source = "Customer"; release = "R1";
        statement = "The user must be able to generate strong passwords with adjustable length and character sets.";
    }
    requirement 'BR-02 Password generation' : BusinessRequirement {
        doc /* Acceptance: length 8..64, A-Z / a-z / 0-9 / symbols toggles,
               exclude ambiguous characters, entropy estimate is shown. */;
    }

    @BusinessRequirementTag {
        id = "BR-03"; priority = "MUST"; source = "Customer"; release = "R1";
        statement = "Credentials must be stored as records: title, login, password, URL, category, note, TOTP secret.";
    }
    requirement 'BR-03 Credential records' : BusinessRequirement {
        doc /* Acceptance: CRUD works for at least 1000 records in one vault. */;
    }

    @BusinessRequirementTag {
        id = "BR-04"; priority = "MUST"; source = "Customer"; release = "R2";
        statement = "Logins and passwords must be autofilled into a desktop app or browser by a hotkey.";
    }
    requirement 'BR-04 Autofill' : BusinessRequirement {
        doc /* Acceptance: Ctrl+Shift+K opens a suggestion list matched by
               domain / window title; clipboard is cleared after 30 s. */;
    }

    @BusinessRequirementTag {
        id = "BR-05"; priority = "MUST"; source = "Customer"; release = "R1";
        statement = "Records must be grouped by categories and found by a fast text search.";
    }
    requirement 'BR-05 Categories and search' : BusinessRequirement {
        doc /* Acceptance: one-level category tree, search over title/login/URL. */;
    }

    @BusinessRequirementTag {
        id = "BR-06"; priority = "SHOULD"; source = "Customer"; release = "R1";
        statement = "Every record must support a free-text secure note.";
    }
    requirement 'BR-06 Secure notes' : BusinessRequirement {
        doc /* Acceptance: up to 2000 characters, encrypted like the password. */;
    }

    @BusinessRequirementTag {
        id = "BR-07"; priority = "MUST"; source = "Customer"; release = "R2";
        statement = "Two-factor authentication must be supported both for stored accounts (TOTP) and for unlocking the vault.";
    }
    requirement 'BR-07 Two-factor authentication' : BusinessRequirement {
        doc /* Acceptance: RFC 6238, 6 digits / 30 s; recovery codes for vault 2FA. */;
    }

    @BusinessRequirementTag {
        id = "BR-08"; priority = "MUST"; source = "Customer"; release = "R2";
        statement = "The user must be able to back up, restore, import and export the vault.";
    }
    requirement 'BR-08 Backup and portability' : BusinessRequirement {
        doc /* Acceptance: encrypted JSON export, CSV import from KeePass/Bitwarden. */;
    }

    @BusinessRequirementTag {
        id = "BR-09"; priority = "SHOULD"; source = "Customer"; release = "R3";
        statement = "The application must warn about weak, reused and outdated passwords.";
    }
    requirement 'BR-09 Password audit' : BusinessRequirement {
        doc /* Acceptance: local report plus optional HIBP k-anonymity check. */;
    }

    @BusinessRequirementTag {
        id = "BR-10"; priority = "MUST"; source = "Customer"; release = "R3";
        statement = "All core functions must work without an Internet connection and without a cloud account.";
    }
    requirement 'BR-10 Offline-first and privacy' : BusinessRequirement {
        doc /* Acceptance: network access is opt-in; no telemetry, no cloud sync in v1. */;
    }

    // -----------------------------------------------------------------
    // 6. Representative derived system requirements (traceable to BR)
    //    Full catalogue with rationale / acceptance / verification:
    //    srs/srs-template.md and srs/srs-template-en.md, section 3.
    // -----------------------------------------------------------------
    metadata def SystemRequirementTag {
        attribute id     : ScalarValues::String;
        attribute area   : ScalarValues::String;
        attribute trace  : ScalarValues::String;
        attribute method : ScalarValues::String;
    }
    requirement def SystemRequirement;

    @SystemRequirementTag { id = "REQ-INT-001";  area = "INT";   trace = "BR-01,BR-07"; method = "Inspection"; }
    requirement 'REQ-INT-001 Four mandatory screens' : SystemRequirement;

    @SystemRequirementTag { id = "REQ-FUNC-001"; area = "FUNC";  trace = "BR-01";       method = "Test"; }
    requirement 'REQ-FUNC-001 Unlock vault' : SystemRequirement;

    @SystemRequirementTag { id = "REQ-FUNC-002"; area = "FUNC";  trace = "BR-01,BR-07"; method = "Test"; }
    requirement 'REQ-FUNC-002 Vault 2FA and recovery key' : SystemRequirement;

    @SystemRequirementTag { id = "REQ-FUNC-003"; area = "FUNC";  trace = "BR-02";       method = "Test"; }
    requirement 'REQ-FUNC-003 Generate password' : SystemRequirement;

    @SystemRequirementTag { id = "REQ-FUNC-004"; area = "FUNC";  trace = "BR-03";       method = "Test"; }
    requirement 'REQ-FUNC-004 Record CRUD' : SystemRequirement;

    @SystemRequirementTag { id = "REQ-FUNC-005"; area = "FUNC";  trace = "BR-05";       method = "Test"; }
    requirement 'REQ-FUNC-005 Categories' : SystemRequirement;

    @SystemRequirementTag { id = "REQ-FUNC-006"; area = "FUNC";  trace = "BR-05";       method = "Test"; }
    requirement 'REQ-FUNC-006 Search' : SystemRequirement;

    @SystemRequirementTag { id = "REQ-FUNC-007"; area = "FUNC";  trace = "BR-06";       method = "Test"; }
    requirement 'REQ-FUNC-007 Secure notes' : SystemRequirement;

    @SystemRequirementTag { id = "REQ-FUNC-008"; area = "FUNC";  trace = "BR-04";       method = "Demonstration"; }
    requirement 'REQ-FUNC-008 Autofill by hotkey' : SystemRequirement;

    @SystemRequirementTag { id = "REQ-FUNC-009"; area = "FUNC";  trace = "BR-07";       method = "Test"; }
    requirement 'REQ-FUNC-009 TOTP codes' : SystemRequirement;

    @SystemRequirementTag { id = "REQ-FUNC-010"; area = "FUNC";  trace = "BR-08";       method = "Test"; }
    requirement 'REQ-FUNC-010 Backup and restore' : SystemRequirement;

    @SystemRequirementTag { id = "REQ-FUNC-011"; area = "FUNC";  trace = "BR-08";       method = "Test"; }
    requirement 'REQ-FUNC-011 Import and export' : SystemRequirement;

    @SystemRequirementTag { id = "REQ-FUNC-012"; area = "FUNC";  trace = "BR-09";       method = "Test"; }
    requirement 'REQ-FUNC-012 Security audit' : SystemRequirement;

    @SystemRequirementTag { id = "REQ-SEC-001";  area = "SEC";   trace = "BR-01";       method = "Analysis"; }
    requirement 'REQ-SEC-001 Argon2id key derivation' : SystemRequirement;

    @SystemRequirementTag { id = "REQ-SEC-002";  area = "SEC";   trace = "BR-01";       method = "Analysis"; }
    requirement 'REQ-SEC-002 AES-256-GCM at rest' : SystemRequirement;

    @SystemRequirementTag { id = "REQ-SEC-005";  area = "SEC";   trace = "BR-04";       method = "Test"; }
    requirement 'REQ-SEC-005 Clipboard clearing' : SystemRequirement;

    @SystemRequirementTag { id = "REQ-SEC-007";  area = "SEC";   trace = "BR-09";       method = "Inspection"; }
    requirement 'REQ-SEC-007 HIBP k-anonymity only' : SystemRequirement;

    @SystemRequirementTag { id = "REQ-COMP-001"; area = "COMP";  trace = "BR-01";       method = "Inspection"; }
    requirement 'REQ-COMP-001 Cryptographic standards' : SystemRequirement;

    @SystemRequirementTag { id = "REQ-POC-001";  area = "POC";   trace = "BR-04";       method = "Demonstration"; }
    requirement 'REQ-POC-001 Autofill feasibility PoC' : SystemRequirement;
}
