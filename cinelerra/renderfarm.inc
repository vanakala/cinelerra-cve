#ifndef RENDERFARM_INC
#define RENDERFARM_INC


class RenderFarmServer;
class RenderFarmServerThread;
class RenderFarmKeepalive;

// Number of seconds before giving up on a socket.
// Change if VFS is slow.
#define RENDERFARM_TIMEOUT 15

#endif
