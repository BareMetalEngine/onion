#pragma once

//--

class ToolAutoRelease
{
public:
    ToolAutoRelease();

    int run(const char* argv0, const Commandline& cmdline);    
};

//--