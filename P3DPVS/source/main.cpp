#include "../include/mainloop.h"
#include "../include/setup.h"

#include <thread>

constexpr int num_threads = 88;
std::map<unsigned int, std::unordered_set<unsigned int>> visData;
std::mutex g_mapMutex;

void StorePVS(const unsigned int node_count);

QString objPath = "C:\\Users\\Zak\\Downloads\\Villa\\Villa.obj";

void merge_maps(std::map<unsigned int, std::unordered_set<unsigned int>>& dst,
                const std::map<unsigned int, std::unordered_set<unsigned int>>& src)
{
    std::lock_guard<std::mutex> lock(g_mapMutex);

    for (const auto& [key, srcSet] : src)
    {
        auto& dstSet = dst[key];
        dstSet.insert(srcSet.begin(), srcSet.end());
    }
}

void worker(unsigned int slice)
{
    Setup setup;
    MainLoop loop;

    setup.DoSetup();
    loop.Run(false, slice, num_threads);

    merge_maps(visData, loop.GetPVSData());
}

int main(int argc, char *argv[])
{
    std::vector<std::thread*> threads;

    for(int i = 1; i < num_threads; i++)
    {
        std::thread* t = new std::thread(worker, i);
        threads.push_back(t);
    }

    Setup setup;
    MainLoop loop;

    setup.DoSetup();
    loop.Run(true, 0, num_threads);

    merge_maps(visData, loop.GetPVSData());

    for(int i = 0; i < threads.size(); i++)
        threads[i]->join();

    StorePVS(loop.GetNodeCount());

    return 0;
}

void SaveBytesAsCFile(QByteArray bytes, QString file);

void StorePVS(const unsigned int node_count)
{
    const unsigned int leaf_count = (node_count * 2) + 1; //We can be front or back of each leaf.

    const unsigned int node_bitmap_len_bytes = (node_count + 7) / 8;

    unsigned char** vis_bitmaps = new unsigned char*[leaf_count];
    unsigned int* vis_offsets = new unsigned int[leaf_count];

    for(int i = 0; i < leaf_count; i++)
    {
        if(visData.contains(i))
        {
            vis_bitmaps[i] = new unsigned char[node_bitmap_len_bytes];
            std::memset(vis_bitmaps[i], 0, node_bitmap_len_bytes);

            std::unordered_set<unsigned int> visSet = visData[i];

            for(int j = 0; j < node_count; j++)
            {
                if(visSet.contains(j))
                {
                    vis_bitmaps[i][j / 8] |= (1 << (j % 8));
                }
            }
        }
        else
            vis_bitmaps[i] = nullptr;
    }

    QByteArray bytes;
    QBuffer buffer(&bytes);
    buffer.open(QIODevice::WriteOnly);

    P3D::VisDataHeader vdh;

    vdh.leaf_count = leaf_count;
    vdh.leaf_index_offset = sizeof(P3D::VisDataHeader);

    buffer.write((const char*)&vdh, sizeof(vdh));

    unsigned int wb = 0;

    for(int i = 0; i < leaf_count; i++)
    {
        if(vis_bitmaps[i])
        {
            vis_offsets[i] = buffer.pos() + (sizeof(unsigned int) * leaf_count) + wb;
            wb+= node_bitmap_len_bytes;
        }
        else
            vis_offsets[i] = 0;
    }

    buffer.write((const char*)vis_offsets, sizeof(unsigned int) * leaf_count);

    for(int i = 0; i < leaf_count; i++)
    {
        if(vis_bitmaps[i])
        {
            buffer.write((const char*)vis_bitmaps[i], node_bitmap_len_bytes);
        }
    }


    QDir workDir = QDir(QFileInfo(objPath).absolutePath());
    QString baseName = QFileInfo(objPath).fileName().chopped(3);

    QFile bspFile(workDir.filePath(baseName + "pvs"));
    bspFile.open(QFile::Truncate | QFile::ReadWrite);
    bspFile.write(bytes);
    bspFile.close();

    SaveBytesAsCFile(bytes, workDir.filePath(baseName + "pvs.cpp"));
}

void SaveBytesAsCFile(QByteArray bytes, QString file)
{
    QFile f(file);

    if(!f.open(QIODevice::Truncate | QIODevice::ReadWrite))
        return;

    QString decl = QString("const extern unsigned char pvsdata[%1UL] = {\n").arg(bytes.size());

    f.write(decl.toLatin1());

    for(int i = 0; i < bytes.size(); i++)
    {
        QString element = QString("0x%1,").arg((quint8)bytes.at(i),2, 16, QChar('0'));

        if(( (i+1) % 40) == 0)
            element += "\n";

        f.write(element.toLatin1());
    }

    QString close = QString("\n};");
    f.write(close.toLatin1());

    f.close();
}
