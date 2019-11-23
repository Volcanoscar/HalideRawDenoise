#include <sys/time.h>
#include <iostream>
#include <map>
#include <vector>
#include <cstring>

class Profiler 
{
public:
	Profiler(const char* in_name, const int in_num_loop = 1)
		: name(in_name), num_loop(in_num_loop)
	{
		gettimeofday(&start, NULL);
	}

	~Profiler()
	{
		gettimeofday(&end, NULL);

		long seconds = end.tv_sec - start.tv_sec;
		long useconds = end.tv_usec - start.tv_usec;

		long mtime = (((seconds) * 1000 + useconds / 1000.0) + 0.5) / num_loop;

        if(num_loop!=1)
            std::cout << std::endl << "[TIME] : " << name << " | " << mtime << " ms | average of " << num_loop << " iterations" << std::endl;
        else
            std::cout << std::endl << "[TIME] : " << name << " | " << mtime << " ms " << std::endl;
	}

private:
    std::string name;
    int32_t num_loop;
	struct timeval start;
	struct timeval end;
};

#define PROFILE_MULTIPLE_ITERATIONS(_t_, _c_)		Profiler tmp(_t_, _c_);
#define PROFILE_ONCE(_t_) Profiler tmp(_t_);

class ConfigReader
{
public:
    ConfigReader(std::string& in_file_path)
        : src_path(in_file_path)
    {
        const char* ifname = in_file_path.c_str();
        FILE* fp = fopen(ifname, "r");
        if (NULL == fp)
        {
            throw std::string("Error opening config file: " + in_file_path); 
        }

        char key[512];
        char value[512];

        while (EOF != fscanf(fp, "%s", key) &&
            EOF != fscanf(fp, "%s", value) )
        {
            Add(std::string(key), std::string(value));
        }

        fclose(fp);
    }

    std::string Get(std::string& in_key)
    {
		std::map<std::string, std::string>::iterator iter = m_map.find(in_key);
		if (iter != m_map.end())
		{
			return iter->second;
		}
		else
		{
			throw std::string("cannot find requested key: "+in_key);
		}
    }

    std::string Get(const char* in_key)
    {
        std::string k(in_key);
        return Get(k);
    }

    int32_t GetInt(const char* in_key)
    {
        std::string s = Get(in_key);
        return atoi(s.c_str());
        // const char* t = s.c_str()
        // return atoi(t);
    }

    void Print()
    {
        std::cerr << "========================" << std::endl;
        std::cerr << "Config file " << src_path << "content: " << std::endl;

		for (std::map<std::string, std::string>::iterator iter = m_map.begin(); iter!=m_map.end(); iter++)
        {
            std::cerr << iter->first << " | " << iter->second << std::endl;
        }
        std::cerr << std::endl << "========================" << std::endl;
    }

private:
    void Add(const std::string& key, const std::string& value)
    {
        m_map.insert(std::pair<std::string, std::string>(key, value));
        std::cout << key << "," << value << std::endl;
    }

private:
    std::map<std::string, std::string> m_map;
    std::string src_path;
};

class FileReader
{
public:
    std::vector<uint8_t*> buffers;

public:
    FileReader(
            std::string dir, 
            std::vector<std::string> &img_names,
            const size_t bin_size
            )
        : buffers(0)
    {
        for (auto i : img_names)
        {
            uint8_t* bin_src = (uint8_t*)malloc(bin_size);

            if(NULL==bin_src)
            {
                std::string err("Error in allocating buffer");
                throw err;
            }
            else
            {
                std::string img_path(dir + "/" + i);

                try
                {
                    read_from_file(img_path, bin_src, bin_size);
                }
                catch(std::string& err)
                {
                    std::cerr << err << std::endl;
                    throw err;
                }

                buffers.push_back(bin_src);
            }
        }
    }

    ~FileReader()
    {
        for (auto i : buffers)
            free(i);
    }

private:
    void read_from_file(std::string& in_fname, uint8_t* buf, const size_t len)
    {
        FILE* fp = fopen(in_fname.c_str(), "rb");
        if(NULL==fp)
        {
            std::string err("Error opening data file: " + in_fname);
            std::cout << err << std::endl;
            throw err;
        }
        if( len != fread(buf, 1, len, fp) )
        {
            std::string err("Error reading data file: " + in_fname);
            std::cout << err << std::endl;
            fclose(fp);
            throw err;
        }
        else
        {
            fclose(fp);
            std::cout << "Load " << in_fname << "..." << std::endl;
        }
    }
};


