#pragma once

typedef unsigned char cc_t;
typedef unsigned int speed_t;
typedef unsigned int tcflag_t;

// * Input Modes
#define BRKINT  0x0001
#define ICRNL   0x0002
#define IGNBRK  0x0004
#define IGNCR   0x0008
#define IGNPAR  0x0010
#define INLCR   0x0020
#define INPCK   0x0040
#define ISTRIP  0x0080
#define IXANY   0x0100
#define IXOFF   0x0200
#define IXON    0x0400
#define PARMRK  0x0800

// * Output Modes
#define OPOST   0x0001
#define ONLCR   0x0002
#define OCRNL   0x0004
#define ONOCR   0x0008
#define ONLRET  0x0010
#define OFILL   0x0020
#define OFDEL   0x0040
#define NLDLY   0x0100
#define CRDLY   0x0200
#define TABDLY  0x0400
#define BSDLY   0x0800
#define VTDLY   0x1000
#define FFDLY   0x2000

// * Control Modes
#define CSIZE   0x0030
#define CS5     0x0000
#define CS6     0x0010
#define CS7     0x0020
#define CS8     0x0030
#define CSTOPB  0x0040
#define CREAD   0x0080
#define PARENB  0x0100
#define PARODD  0x0200
#define HUPCL   0x0400
#define CLOCAL  0x0800
#define B0      0x0000
#define B50     0x0001
#define B75     0x0002
#define B110    0x0003
#define B134    0x0004
#define B150    0x0005
#define B200    0x0006
#define B300    0x0007
#define B600    0x0008
#define B1200   0x0009
#define B1800   0x000A
#define B2400   0x000B
#define B4800   0x000C
#define B9600   0x000D
#define B19200  0x000E
#define B38400  0x000F

// * Local Modes
#define ISIG    0x0001
#define ICANON  0x0002
#define ECHO    0x0004
#define ECHOE   0x0008
#define ECHOK   0x0010
#define ECHONL  0x0020
#define NOFLSH  0x0040
#define TOSTOP  0x0080
#define IEXTEN  0x0100

#define NCCS 32

#define VINTR     0
#define VQUIT     1
#define VERASE    2
#define VKILL     3
#define VEOF      4
#define VTIME     5
#define VMIN      6
#define VSWTC     7
#define VSTART    8
#define VSTOP     9
#define VSUSP     10
#define VEOL      11
#define VREPRINT  12
#define VDISCARD  13
#define VWERASE   14
#define VLNEXT    15
#define VEOL2     16

struct termios
{
    tcflag_t  c_iflag;     // Input modes.
    tcflag_t  c_oflag;     // Output modes.
    tcflag_t  c_cflag;     // Control modes.
    tcflag_t  c_lflag;     // Local modes.
    cc_t      c_cc[NCCS];  // Control characters.
};

// * Attribute Selection
#define TCSANOW     1
#define TCSADRAIN   2
#define TCSAFLUSH   3

int tcgetattr(int fildes, struct termios* termios_p);
int tcsetattr(int fildes, int optional_actions, const struct termios* termios_p);