
#define RT_OFFSETOF(type, member)  __builtin_offsetof (type, member)

/* defined in HGSMIChSetup.h */
#define HGSMI_CC_HOST_FLAGS_LOCATION 0

typedef uint32_t HGSMISIZE;
typedef uint32_t HGSMIOFFSET;

typedef struct HGSMIBUFFERLOCATION
{
    HGSMIOFFSET offLocation;
    HGSMISIZE   cbLocation;
} HGSMIBUFFERLOCATION;


#define HGSMIOFFSET_VOID ((HGSMIOFFSET)~0)

/* HGSMI setup and configuration data structures. */
/* host->guest commands pending, should be accessed under FIFO lock only */
#define HGSMIHOSTFLAGS_COMMANDS_PENDING    UINT32_C(0x01)
/* IRQ is fired, should be accessed under VGAState::lock only  */
#define HGSMIHOSTFLAGS_IRQ                 UINT32_C(0x02)
#ifdef VBOX_WITH_WDDM
/* one or more guest commands is completed, should be accessed under FIFO lock only */
# define HGSMIHOSTFLAGS_GCOMMAND_COMPLETED UINT32_C(0x04)
/* watchdog timer interrupt flag (used for debugging), should be accessed under VGAState::lock only */
# define HGSMIHOSTFLAGS_WATCHDOG           UINT32_C(0x08)
#endif
/* vsync interrupt flag, should be accessed under VGAState::lock only */
#define HGSMIHOSTFLAGS_VSYNC               UINT32_C(0x10)
/** monitor hotplug flag, should be accessed under VGAState::lock only */
#define HGSMIHOSTFLAGS_HOTPLUG             UINT32_C(0x20)
/**
 * Cursor capability state change flag, should be accessed under
 * VGAState::lock only.  @see VBVACONF32.
 */
#define HGSMIHOSTFLAGS_CURSOR_CAPABILITIES UINT32_C(0x40)

typedef struct HGSMIHOSTFLAGS
{
    /*
     * Host flags can be accessed and modified in multiple threads
     * concurrently, e.g. CrOpenGL HGCM and GUI threads when completing
     * HGSMI 3D and Video Accel respectively, EMT thread when dealing with
     * HGSMI command processing, etc.
     * Besides settings/cleaning flags atomically, some flags have their
     * own special sync restrictions, see comments for flags above.
     */
    volatile uint32_t u32HostFlags;
    uint32_t au32Reserved[3];
} HGSMIHOSTFLAGS;


/* defined in HGSMIDefs.h */

/* The buffer description flags. */
#define HGSMI_BUFFER_HEADER_F_SEQ_MASK     0x03 /* Buffer sequence type mask. */
#define HGSMI_BUFFER_HEADER_F_SEQ_SINGLE   0x00 /* Single buffer, not a part of a sequence. */
#define HGSMI_BUFFER_HEADER_F_SEQ_START    0x01 /* The first buffer in a sequence. */
#define HGSMI_BUFFER_HEADER_F_SEQ_CONTINUE 0x02 /* A middle buffer in a sequence. */
#define HGSMI_BUFFER_HEADER_F_SEQ_END      0x03 /* The last buffer in a sequence. */

#pragma pack(1) /** @todo not necessary. use AssertCompileSize instead. */
/* 16 bytes buffer header. */
typedef struct HGSMIBUFFERHEADER
{
    uint32_t    u32DataSize;            /* Size of data that follows the header. */

    uint8_t     u8Flags;                /* The buffer description: HGSMI_BUFFER_HEADER_F_* */

    uint8_t     u8Channel;              /* The channel the data must be routed to. */
    uint16_t    u16ChannelInfo;         /* Opaque to the HGSMI, used by the channel. */

    union {
        uint8_t au8Union[8];            /* Opaque placeholder to make the union 8 bytes. */

        struct
        {                               /* HGSMI_BUFFER_HEADER_F_SEQ_SINGLE */
            uint32_t u32Reserved1;      /* A reserved field, initialize to 0. */
            uint32_t u32Reserved2;      /* A reserved field, initialize to 0. */
        } Buffer;

        struct
        {                               /* HGSMI_BUFFER_HEADER_F_SEQ_START */
            uint32_t u32SequenceNumber; /* The sequence number, the same for all buffers in the sequence. */
            uint32_t u32SequenceSize;   /* The total size of the sequence. */
        } SequenceStart;

        struct
        {                               /* HGSMI_BUFFER_HEADER_F_SEQ_CONTINUE and HGSMI_BUFFER_HEADER_F_SEQ_END */
            uint32_t u32SequenceNumber; /* The sequence number, the same for all buffers in the sequence. */
            uint32_t u32SequenceOffset; /* Data offset in the entire sequence. */
        } SequenceContinue;
    } u;
} HGSMIBUFFERHEADER;

/* 8 bytes buffer tail. */
typedef struct HGSMIBUFFERTAIL
{
    uint32_t    u32Reserved;        /* Reserved, must be initialized to 0. */
    uint32_t    u32Checksum;        /* Verifyer for the buffer header and offset and for first 4 bytes of the tail. */
} HGSMIBUFFERTAIL;


/* defined in HGSMIChannels.h */

/* Predefined channel identifiers. Used internally by VBOX to simplify the channel setup. */
/* A reserved channel value */
#define HGSMI_CH_RESERVED     0x00
/* HGCMI: setup and configuration */
#define HGSMI_CH_HGSMI        0x01
/* Graphics: VBVA */
#define HGSMI_CH_VBVA         0x02
/* Graphics: Seamless with a single guest region */
#define HGSMI_CH_SEAMLESS     0x03
/* Graphics: Seamless with separate host windows */
#define HGSMI_CH_SEAMLESS2    0x04
/* Graphics: OpenGL HW acceleration */
#define HGSMI_CH_OPENGL       0x05

void InitializeHeader(HGSMIBUFFERHEADER *, HGSMISIZE, uint8_t, uint16_t, uint32_t);

