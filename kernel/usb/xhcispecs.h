#pragma once

#include <kernel/types.h>

enum {
    USBCMD_RS     = (1 << 0),  // Run/Stop
    USBCMD_HCRST  = (1 << 1),  // Host Controller Reset
    USBCMD_INTE   = (1 << 2),  // Interrupter Enable
    USBCMD_HSEE   = (1 << 3),  // Host System Error Enable
    USBCMD_LHCRST = (1 << 7),  // Light Host Controller Reset
    USBCMD_CSS    = (1 << 8),  // Controller Save State
    USBCMD_CRS    = (1 << 9),  // Controller Restore State
    USBCMD_EWE    = (1 << 10), // Enable Wrap Event
    USBCMD_EU3S   = (1 << 11), // Enable U3 MFINDEX Stop
    USBCMD_CME    = (1 << 13), // CEM Enable
    USBCMD_ETE    = (1 << 14), // Extended TBC Enable
    USBCMD_TSCEN  = (1 << 15), // Extended TBC TRB Status Enable
    USBCMD_VTIOEN = (1 << 16), // VTIO Enable
};

enum {
    USBSTS_HCH    = (1 << 0),  // Host Controller Halted
    USBSTS_HSE    = (1 << 2),  // Host System Error
    USBSTS_EINT   = (1 << 3),  // Event Interrupt
    USBSTS_PCD    = (1 << 4),  // Port Change Detect
    USBSTS_SSS    = (1 << 8),  // Save State Status
    USBSTS_RSS    = (1 << 9),  // Restore State Status
    USBSTS_SRE    = (1 << 10), // Save/Restore Error
    USBSTS_CNR    = (1 << 11), // Controller Not Ready
    USBSTS_HCE    = (1 << 12), // Host Controller Error
};

enum {
    CRCR_RCS = (1 << 0), // Ring Cycle State
    CRCR_CS  = (1 << 1), // Command Stop
    CRCR_CA  = (1 << 2), // Command Abort
    CRCR_CRR = (1 << 3), // Command Ring Running
};

enum {
    IMAN_IP = (1 << 0), // Interrupt Pending
    IMAN_IE = (1 << 1), // Interrupt Enable
};

enum {
    // Capability Parameters 1
    CAP_AC64  = (1 << 0),  // 64-bit Addressing Capability
    CAP_BNC   = (1 << 1),  // BW Negotiation Capability
    CAP_CSZ   = (1 << 2),  // Context Size
    CAP_PPC   = (1 << 3),  // Port Power Control
    CAP_PIND  = (1 << 4),  // Port Indicators
    CAP_LHRC  = (1 << 5),  // Light HC Reset Capability
    CAP_LTC   = (1 << 6),  // Latency Tolerance Messaging Capability
    CAP_NSS   = (1 << 7),  // No Secondary SID Support
    CAP_PAE   = (1 << 8),  // Parse All Event Data
    CAP_SPC   = (1 << 9),  // Stopped - Short Packet Capability
    CAP_SEC   = (1 << 10), // Stopped EDTLA Capability
    CAP_CFC   = (1 << 11), // Contiguous Frame ID Capability

    // Capability Parameters 2
    CAP_U3C   = (1 << 0),  // U3 Entry Capability
    CAP_BMC   = (1 << 1),  // Configure Endpoint Command Max Exit Latency Too Large Capability
    CAP_FSC   = (1 << 2),  // Force Save Context Capability
    CAP_CTC   = (1 << 3),  // Compliance Transition Capability
    CAP_LEC   = (1 << 4),  // Large ESIT Payload Capability
    CAP_CIC   = (1 << 5),  // Configuration Information Capability
    CAP_ETC   = (1 << 6),  // Extended TBC Capability
    CAP_TSC   = (1 << 7),  // Extended TBC TRB Status Capability
    CAP_GSC   = (1 << 8),  // Get/Set Extended Property Capability
    CAP_VTC   = (1 << 9),  // Virtualization Based Trusted I/O Capability
};

enum {
    PORTSC_CCS = (1 << 0),  // Current Connect Status
    PORTSC_PED = (1 << 1),  // Port Enabled/Disabled
    PORTSC_OCA = (1 << 3),  // Over-current Active
    PORTSC_PR  = (1 << 4),  // Port Reset
    PORTSC_PP  = (1 << 9),  // Port Power
    PORTSC_LWS = (1 << 16), // Port Link State Write Strobe
    PORTSC_CSC = (1 << 17), // Connect Status Change
    PORTSC_PEC = (1 << 18), // Port Enabled/Disabled Change
    PORTSC_WRC = (1 << 19), // Warm Port Reset Change
    PORTSC_OCC = (1 << 20), // Over-current Change
    PORTSC_PRC = (1 << 21), // Port Reset Change
    PORTSC_PLC = (1 << 22), // Port Link State Change
    PORTSC_CEC = (1 << 23), // Port Config Error Change
    PORTSC_CAS = (1 << 24), // Cold Attach Status
    PORTSC_WCE = (1 << 25), // Wake on Connect Enable
    PORTSC_WDE = (1 << 26), // Wake on Disconnect Enable
    PORTSC_WOE = (1 << 27), // Wake on Over-current Enable
    PORTSC_DR  = (1 << 30), // Device Removable
    PORTSC_WPR = (1 << 31), // Warm Port Reset
};

// TRB Type IDs
enum {
    // Transfers
    TRB_TYPE_NORMAL                = 1,
    TRB_TYPE_SETUP_STAGE           = 2,
    TRB_TYPE_DATA_STAGE            = 3,
    TRB_TYPE_STATUS_STAGE          = 4,
    TRB_TYPE_ISOCH                 = 5,
    TRB_TYPE_LINK                  = 6,
    TRB_TYPE_EVENT_DATA            = 7,
    TRB_TYPE_TR_NOOP               = 8,

    // Commands
    TRB_TYPE_ENABLE_SLOT           = 9,
    TRB_TYPE_DISABLE_SLOT          = 10,
    TRB_TYPE_ADDRESS_DEVICE        = 11,
    TRB_TYPE_CONFIGURE_ENDPOINT    = 12,
    TRB_TYPE_EVALUATE_CONTEXT      = 13,
    TRB_TYPE_RESET_ENDPOINT        = 14,
    TRB_TYPE_STOP_ENDPOINT         = 15,
    TRB_TYPE_SET_TR_DEQUEUE        = 16,
    TRB_TYPE_RESET_DEVICE          = 17,
    TRB_TYPE_FORCE_EVENT           = 18,
    TRB_TYPE_NEGOTIATE_BANDWIDTH   = 19,
    TRB_TYPE_SET_LATENCY_TOLERANCE = 20,
    TRB_TYPE_GET_PORT_BANDWIDTH    = 21,
    TRB_TYPE_FORCE_HEADER          = 22,
    TRB_TYPE_CMD_NOOP              = 23,

    // Events
    TRB_TYPE_TRANSFER              = 32,
    TRB_TYPE_COMMAND_COMPLETION    = 33,
    TRB_TYPE_PORT_STATUS_CHANGE    = 34,
    TRB_TYPE_BANDWIDTH_REQUEST     = 35,
    TRB_TYPE_DOORBELL              = 36,
    TRB_TYPE_HOST_CONTROLLER       = 37,
    TRB_TYPE_DEVICE_NOTIFICATION   = 38,
    TRB_TYPE_MFINDEX_WRAP          = 39,
};

// TRB Flags
enum {
    TRB_CYCLE = (1 << 0),  // Cycle
    TRB_ENT   = (1 << 1),  // Evaluate Next TRB
    TRB_ISP   = (1 << 2),  // Interrupt-on Short Packet
    TRB_NS    = (1 << 3),  // No Snoop
    TRB_CHAIN = (1 << 4),  // Chain bit
    TRB_IOC   = (1 << 5),  // Interrupt On Completion
    TRB_IDT   = (1 << 6),  // Immediate Data
    TRB_BEI   = (1 << 9),  // Block Event Interrupt
};

// TRB Transfer Type
enum {
    TRT_NO_DATA  = 0,
    TRT_OUT_DATA = 2,
    TRT_IN_DATA  = 3,
};

// Endpoint Type for Endpoint Context
enum {
    EP_TYPE_ISOCHRONOUS_OUT = 1,
    EP_TYPE_BULK_OUT        = 2,
    EP_TYPE_INTERRUPT_OUT   = 3,
    EP_TYPE_CONTROL         = 4,
    EP_TYPE_ISOCHRONOUS_IN  = 5,
    EP_TYPE_BULK_IN         = 6,
    EP_TYPE_INTERRUPT_IN    = 7,
};

// Structural Parameters 1
typedef struct {
    uint32_t maxslots : 8;  // Number of Device Slots
    uint32_t maxintrs : 11; // Number of Interrupters
    uint32_t reserved : 5;  // Reserved
    uint32_t maxports : 8;  // Number of Ports
} __attribute__((packed)) xhci_hcsparams1_t;

// Structural Parameters 2
typedef struct {
    uint32_t ist          : 4;  // Isochronous Scheduling Threshold
    uint32_t erstmax      : 4;  // Event Ring Segment Table Max
    uint32_t reserved     : 13; // Reserved
    uint32_t maxscpbufshi : 5;  // Max Scratchpad Buffers (High 5 bits)
    uint32_t srp          : 1;  // Scratchpad Restore
    uint32_t maxscpbufslo : 5;  // Max Scratchpad Buffers (Low 5 bits)
} __attribute__((packed)) xhci_hcsparams2_t;

// Structural Parameters 3
typedef struct {
    uint32_t u1exitlat : 8;   // U1 Device Exit Latency
    uint32_t reserved  : 8;   // Reserved
    uint32_t u2exitlat : 16;  // U2 Device Exit Latency
} __attribute__((packed)) xhci_hcsparams3_t;

// Host Controller Capability Register
typedef volatile struct {
    uint32_t capver;      // Capability Register Length (0:7) and Interface Version Number (16:31)
    uint32_t hcsparams1;  // Structural Parameters 1
    uint32_t hcsparams2;  // Structural Parameters 2
    uint32_t hcsparams3;  // Structural Parameters 3
    uint32_t hccparams1;  // Capability Parameters 1
    uint32_t dboff;       // Doorbell Offset
    uint32_t rtsoff;      // Runtime Register Space Offset
    uint32_t hccparams2;  // Capability Parameters 2
} __attribute__((packed)) xhci_hcc_t;

// Host Controller USB Port Register
typedef volatile struct {
    uint32_t portsc;    // Port Status and Control
    uint32_t portpmsc;  // Port Power Management Status and Control
    uint32_t portli;    // Port Link Info
    uint32_t reserved;  // Reserved
} __attribute__((packed)) xhci_hcp_t;

// Host Controller Operational Register
typedef volatile struct {
    uint32_t usbcmd;         // USB Command
    uint32_t usbsts;         // USB Status
    uint32_t pagesize;       // Page Size
    uint64_t reserved0;      // Reserved
    uint32_t dncrtl;         // Device Notification Control
    uint64_t crcr;           // Command Ring Control Register
    uint64_t reserved1;      // Reserved
    uint64_t reserved2;      // Reserved
    uint64_t dcbaap;         // Device Context Base Address Array Pointer
    uint32_t config;         // Configure
    uint8_t reserved3[964];  // Reserved
    xhci_hcp_t port[];       // Port Registers
} __attribute__((packed)) xhci_hco_t;

// Doorbell Registers
typedef volatile struct {
    uint32_t value[256]; // DB Target (0:7) and DB Task ID (16:31)
} __attribute__((packed)) xhci_db_t;

// Interrupter Register
typedef volatile struct {
    uint32_t iman;      // Interrupter Management
    uint32_t imod;      // Interrupter Moderation
    uint32_t erstsz;    // Event Ring Segment Table Size
    uint32_t reserved;  // Reserved
    uint64_t erstba;    // Event Ring Segment Table Base Address
    uint64_t erdp;      // Event Ring Dequeue Pointer
} __attribute__((packed)) xhci_hci_t;

// Host Controller Runtime Register
typedef volatile struct {
    uint32_t mfindex;      // Microframe Index
    uint8_t reserved[28];  // Reserved
    xhci_hci_t intr[];     // Interrupter Registers
} __attribute__((packed)) xhci_hcr_t;

// Event Ring Segment Table Entry
typedef struct {
    uint64_t rsba;     // Ring Segment Base Address
    uint32_t rss;      // Ring Segment Size (low 16 bits, upper 16 bits should be zero)
    uint32_t reserved; // Reserved
} __attribute__((packed)) xhci_erste_t;

// Transfer Request Block
typedef struct {
    uint64_t address; // Data Buffer Pointer
    uint32_t status;  // TRB Transfer Length (0:16), TD Size (17:21), Interrupter Target (22:31)
    uint32_t flags;   // Flags (0:9) and Type (10:15)
} __attribute__((packed)) xhci_trb_t;

// Input Control Context Data Structure
typedef struct {
    uint32_t drop_flags;
    uint32_t add_flags;
    uint32_t resv0[5];
    uint32_t configuration_value : 8;
    uint32_t interface_number    : 8;
    uint32_t alternate_setting   : 8;
    uint64_t resv1               : 8;
} __attribute__((packed)) xhci_input_control_context_t;

// Slot Context Data Structure
typedef struct {
    uint32_t route_string     : 20; // Route String
    uint32_t speed            : 4;  // Port Speed (Deprecated, should be considered reserved)
    uint32_t resv0            : 1;  // Reserved
    uint32_t mtt              : 1;  // Multi-TT
    uint32_t hub              : 1;  // Hub
    uint32_t entries          : 5;  // Index of last valid Endpoint Context
    uint32_t max_exit_latency : 16; // Max Exit Latency in microseconds
    uint32_t root_hub_port    : 8;  // Root Hub Port Number
    uint32_t number_of_ports  : 8;  // Number of downstream ports if this device is a Hub
    uint32_t tt_hub_slot_id   : 8;  // Parent Hub Slot ID
    uint32_t tt_port          : 8;  // Parent Port Number
    uint32_t ttt              : 2;  // TT Think Time
    uint32_t resv1            : 4;  // Reserved
    uint32_t intr_target      : 10; // Interrupter Target
    uint32_t device_address   : 8;  // USB Device Address
    uint32_t resv2            : 19; // Reserved
    uint32_t slot_state       : 5;  // Slot State
} __attribute__((packed)) xhci_slot_context_t;

// Endpoint Context Data Structure
typedef struct {
    uint32_t ep_state         : 3;  // Endpoint State
    uint32_t resv0            : 5;  // Reserved
    uint32_t mult             : 2;  // Mult
    uint32_t max_p_streams    : 5;  // Max Primary Streams
    uint32_t lsa              : 1;  // Linear Stream Array
    uint32_t interval         : 8;  // Interval (in units of 125 us times 2^N)
    uint32_t max_payload_hi   : 8;  // Max Endpoint Service Time Interval (ESIT) Payload High
    uint32_t resv1            : 1;  // Reserved
    uint32_t cerr             : 2;  // Error Count
    uint32_t ep_type          : 3;  // Endpoint Type
    uint32_t resv2            : 1;  // Reserved
    uint32_t hid              : 1;  // Host Initiate Disable
    uint32_t max_burst_size   : 8;  // Max Burst Size
    uint32_t max_packet_size  : 16; // Max Packet Size
    uint64_t tr_dequeue_ptr;        // TR Dequeue Pointer (16-byte aligned), Dequeue Cycle State (bit 0)
    uint32_t avg_trb_length   : 16; // Average TRB Length
    uint32_t max_payload_lo   : 16; // Max Endpoint Service Time Interval (ESIT) Payload Low
} __attribute__((packed)) xhci_endpoint_context_t;

//
// Support structures that are not directly defined in the specification
//

// Ring (Command, Transfer, Event)
typedef volatile struct {
    xhci_trb_t *trb;    // TRB List
    xhci_trb_t *latest; // Latest Event
    uint64_t phys;      // Physical Address of TRB List
    uint32_t index;     // Index into TRB List
    uint32_t ntrb;      // Total number of TRBs
    uint32_t cycle;     // Cycle
} xhci_ring_t;

// Endpoint
typedef void(*xhci_handler_t)(void*);
typedef struct {
    xhci_endpoint_context_t *ctx; // Endpoint Context
    xhci_ring_t ring;             // Transfer or Command Ring
    int dci;                      // Device Context Index
    void *data;                   // Data for Event Handler
    xhci_handler_t handler;       // Event Handler
} xhci_endpoint_t;

// Device Context
typedef struct {
    uint64_t output_phys;                // Physical address of output context
    uint64_t input_phys;                 // Physical address of input context
    xhci_input_control_context_t *ctrl;  // Input Control Context
    xhci_slot_context_t *slot;           // Slot Context
    xhci_endpoint_context_t *ep0;        // Default Control Endpoint
    xhci_ring_t *cmd;                    // Command Ring
    xhci_endpoint_t ep[31];              // All Endpoints
} xhci_context_t;

// XHCI Port
typedef struct {
    int major;
    int minor;
    xhci_hcp_t *hcp;
} xhci_port_t;

// XHCI Controller
typedef struct {
    xhci_hcc_t *hcc;      // Capabilities
    xhci_hco_t *hco;      // Operational
    xhci_hcr_t *hcr;      // Runtime
    xhci_db_t *db;        // Doorbell
    int maxports;         // Max Ports
    int maxints;          // Max Interrupters
    int maxslots;         // Max Device Slots
    int maxerst;          // Event Ring Segment Table Max (ERST Max)
    int maxscpbufs;       // Max Scratchpad Buffers
    int ctxsize;          // Size of Context Device Data Structures
    int version;          // Version
    uint64_t *dcba;       // Device Context Base Address
    xhci_erste_t *erst;   // Event Ring Segment Table
    xhci_ring_t cmd;      // Command Ring
    xhci_ring_t event;    // Event Ring
    xhci_port_t *port;    // Ports
    xhci_context_t *ctx;  // Context for all device slots
    void *hub;
} xhci_t;
