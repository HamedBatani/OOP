//PAGESIZE.H
#pragma once

enum class PageSizeType {
    A4,
    A3,
    Custom
};

struct PageSize {
    double width;
    double height;
    PageSizeType type;
};

const char* pageSizeTypeToString(PageSizeType type);