#pragma once

struct DebugState {
    // Player state
    float* playerPos;      // float[3]
    float* playerRot;      // float[3]
    float* playerColor;    // float[4]

    // Thread node state
    int* threadNodeCount;
    float* threadNodes;    // float[N * 3], N = *threadNodeCount

    // Enemy state (flat arrays)
    int* enemyCount;
    float* enemyPositions; // float[MaxEnemies * 3]
    float* enemyRotations; // float[MaxEnemies * 3]
    float* enemyHPs;       // float[MaxEnemies]

    // Replay state
    bool* isPlayback;
    int* playbackFrame;
    int* totalFrames;

    // Socket sync state
    bool* isSocketSyncEnabled;
    bool* isSocketConnected;
    int* socketPort;

    // Bug trigger and actions
    bool* isBugTriggered;
    char* bugMessage;      // char[256]
    bool* triggerBugNow;
    bool* saveReplayNow;
    bool* loadReplayNow;

    // Additional scene details
    int* staticObjectCount;

    // Allocator functions to share the heap between EXE and DLL
    void* allocFunc;
    void* freeFunc;
    void* allocUserData;
};
