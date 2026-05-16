// SW/LOCO_FIRME_V1.0.3/src/network.h
#pragma once

void networkInit();
bool networkConnected();
void networkLoop(); // gọi trong loop() để reconnect nếu mất kết nối
