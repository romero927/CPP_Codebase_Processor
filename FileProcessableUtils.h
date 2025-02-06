#pragma once

#include <QString>

// Function to determine if a file is processable
// This function can be used across different translation units
bool isFileProcessableImpl(const QString& filePath);