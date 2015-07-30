//  Copyright [2015] <lgb (LiuGuangBao)>
//=====================================================================================
//
//      Filename:  main.cpp
//
//   Description:  测试主程序
//
//       Version:  1.0
//       Created:  2015年07月29日 17时34分46秒
//      Revision:  none
//      Compiler:  gcc
//
//        Author:  lgb (LiuGuangBao), easyeagel@gmx.com
//  Organization:  ezbty.org
//
//=====================================================================================
//

#include<fstream>
#include<core/http/mutilpart.hpp>

int main()
{
    uint32_t fileIdx=0;
    std::ofstream file;

    char buf[1];
    core::MutilpartData md;
    md.boundarySet("------------------------2d34c3d892363d37");
    md.partCallSet([&file, &fileIdx](const core::MutilpartData::PartTrait& trait, const core::Byte* bt, size_t sz)
        {
            if(trait.stat==core::MutilpartData::eStatusStart)
            {
                file.close();

                char filename[256];
                std::snprintf(filename, sizeof(filename), "mp_%ud.pdf", fileIdx++);
                file.open(filename);
            }

            if(bt && sz>0)
                file.write(reinterpret_cast<const char*>(bt), sz);
        }
    );

    std::ifstream stm("out.txt");

    core::ErrorCode ec;
    while(stm.read(buf, sizeof(buf)))
        md.parse(ec, reinterpret_cast<const core::Byte*>(&buf[0]), 1);

    assert(md.isCompleted());
    return 0;
}

