/* EdidService native implementation for the Samsung Q90T (Tizen 5.5).
   Reads EDID from /dev/video28 via V4L2_CID_DV_G_EDID, parses monitor names,
   serves {"hdmi1":...} on http://127.0.0.1:8787/, and writes a trace to
   /opt/usr/apps/EdidServi1/e.status. Runs forever as a native service app. */
#define _GNU_SOURCE
#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>

#define V4L2_CTRL_CLASS_DV     0x00a00000
#define V4L2_CID_DV_CLASS_BASE (V4L2_CTRL_CLASS_DV | 0x900)
#define V4L2_CID_DV_G_EDID     (V4L2_CID_DV_CLASS_BASE + 174)
#define VIDIOC_G_EXT_CTRLS     _IOWR('V', 71, struct v4l2_ext_controls)

struct v4l2_ext_control {
    uint32_t id;
    uint32_t size;
    uint32_t reserved2[1];
    union {
        int32_t value;
        int64_t value64;
        char *string;
        uint8_t *p_u8;
        uint16_t *p_u16;
        uint32_t *p_u32;
    };
};
struct v4l2_ext_controls {
    uint32_t ctrl_class;
    uint32_t count;
    uint32_t error_idx;
    uint32_t reserved[2];
    struct v4l2_ext_control *controls;
};
struct edid_req_std {
    uint32_t pad;
    uint32_t start_block;
    uint32_t blocks;
    uint32_t reserved[5];
    uint8_t *edid;
};
struct edid_req_tztv {
    uint32_t pad;
    uint32_t lenth;
    uint8_t *edid;
};

static const char *TRACE = "/opt/usr/apps/EdidServi1/e.status";

static void trace(const char *msg) {
    int fd = open(TRACE, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (fd >= 0) { write(fd, msg, strlen(msg)); write(fd, "\n", 1); close(fd); }
}

static int is_edid(const uint8_t *b) {
    return b[0] == 0x00 && b[1] == 0xFF && b[2] == 0xFF && b[3] == 0xFF &&
           b[4] == 0xFF && b[5] == 0xFF && b[6] == 0xFF && b[7] == 0x00;
}

static int try_read(int fd, struct v4l2_ext_controls *ctrls,
                    struct v4l2_ext_control *ctl, uint8_t *edid) {
    ctrls->error_idx = 0;
    memset(edid, 0, 256);
    return ioctl(fd, VIDIOC_G_EXT_CTRLS, ctrls) == 0 && is_edid(edid);
}

/* Returns the monitor name (0xFC descriptor) or NULL. Caller frees. */
static char *read_name(int port) {
    int fd = open("/dev/video28", O_RDWR);
    if (fd < 0) { trace("open video28 failed"); return NULL; }
    struct v4l2_ext_control ctl;
    struct v4l2_ext_controls ctrls;
    struct edid_req_std std;
    struct edid_req_tztv tz;
    uint8_t edid[256];
    char *name = NULL;
    int ok = 0;

    memset(&ctl, 0, sizeof(ctl));
    ctl.id = V4L2_CID_DV_G_EDID;
    ctrls.ctrl_class = V4L2_CTRL_CLASS_DV;
    ctrls.count = 1;
    ctrls.error_idx = 0;
    ctrls.reserved[0] = ctrls.reserved[1] = 0;
    ctrls.controls = &ctl;

    memset(&std, 0, sizeof(std));
    std.pad = (uint32_t)port; std.start_block = 0; std.blocks = 1; std.edid = edid;
    ctl.string = (char *)&std;
    if (try_read(fd, &ctrls, &ctl, edid)) ok = 1;
    if (!ok) {
        memset(&tz, 0, sizeof(tz));
        tz.pad = (uint32_t)port; tz.lenth = 256; tz.edid = edid;
        ctl.string = (char *)&tz;
        if (try_read(fd, &ctrls, &ctl, edid)) ok = 1;
    }
    close(fd);
    if (!ok) return NULL;

    for (int off = 54; off <= 108; off += 18) {
        if (edid[off] == 0x00 && edid[off + 1] == 0x00 && edid[off + 3] == 0xFC) {
            char buf[14];
            int n = 0;
            for (int i = off + 5; i < off + 18 && edid[i] != 0x0A && edid[i] != 0x00; i++)
                buf[n++] = (char)edid[i];
            while (n > 0 && (buf[n - 1] == ' ' || buf[n - 1] == '\t')) n--;
            if (n > 0) { name = strndup(buf, (size_t)n); break; }
        }
    }
    return name;
}

static void handle_client(int cfd, const char *json, size_t len) {
    char req[512];
    ssize_t n = recv(cfd, req, sizeof(req) - 1, 0);
    (void)n;
    char head[256];
    int hl = snprintf(head, sizeof(head),
        "HTTP/1.0 200 OK\r\nContent-Type: application/json\r\nContent-Length: %zu\r\n\r\n", len);
    send(cfd, head, (size_t)hl, MSG_NOSIGNAL);
    send(cfd, json, len, MSG_NOSIGNAL);
    close(cfd);
}

int main(void) {
    trace("native service start");
    char json[512];
    int pos = snprintf(json, sizeof(json), "{");
    for (int port = 0; port < 4; port++) {
        char *name = read_name(port);
        if (port > 0) pos += snprintf(json + pos, sizeof(json) - (size_t)pos, ",");
        if (name) {
            pos += snprintf(json + pos, sizeof(json) - (size_t)pos,
                            "\"hdmi%d\":\"%s\"", port + 1, name);
            free(name);
        } else {
            pos += snprintf(json + pos, sizeof(json) - (size_t)pos,
                            "\"hdmi%d\":null", port + 1);
        }
    }
    pos += snprintf(json + pos, sizeof(json) - (size_t)pos, "}");
    trace(json);

    int sfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sfd < 0) { trace("socket failed"); return 1; }
    int one = 1;
    setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port = htons(8787);
    if (bind(sfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) { trace("bind failed"); return 1; }
    if (listen(sfd, 4) < 0) { trace("listen failed"); return 1; }
    trace("listening on 8787");

    for (;;) {
        int cfd = accept(sfd, NULL, NULL);
        if (cfd < 0) continue;
        handle_client(cfd, json, (size_t)pos);
    }
}
