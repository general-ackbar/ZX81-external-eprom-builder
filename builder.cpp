// build_zx81_rom.cpp
#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <vector>
#include <string>
#include <fstream>
#include <iostream>

#include <stdexcept>
#include <algorithm>
#include <cctype>
#include <cstddef>

#include <unistd.h>   // getopt
#include <sys/stat.h>
#include "base.h"
#include "loader.h"


extern "C" {
    #include "zx7/zx7.h"
}


std::vector<unsigned char> zx7_encode(const std::vector<unsigned char>& raw) {
    if (raw.empty()) throw std::runtime_error("empty input");

    std::vector<unsigned char> buf = raw;

    long  skip  = 0;
    long  delta = 0;
    size_t out_sz = 0;

    Optimal* opt = optimize(buf.data(), buf.size(), skip); 
    unsigned char* out = compress(opt, buf.data(), buf.size(),
                                  skip, &out_sz, &delta);
    if (!out || out_sz == 0) {
        if (opt) free(opt);
        throw std::runtime_error("ZX7 compress failed");
    }
    std::vector<unsigned char> res(out, out + out_sz);
    free(out);
    free(opt);
    return res;
}



static std::vector<uint8_t> slurp(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) { throw std::runtime_error("Cannot open: " + path); }
    std::vector<uint8_t> data((std::istreambuf_iterator<char>(f)),
                               std::istreambuf_iterator<char>());
    return data;
}

static bool ends_with_case_insensitive(const std::string& s, const std::string& suffix) {
    if (s.size() < suffix.size()) return false;
    for (size_t i = 0; i < suffix.size(); ++i) {
        char a = std::tolower((unsigned char)s[s.size() - suffix.size() + i]);
        char b = std::tolower((unsigned char)suffix[i]);
        if (a != b) return false;
    }
    return true;
}

static bool is_zx7(const std::string& inputPath) {
    return ends_with_case_insensitive(inputPath, ".zx7");
}

static std::vector<uint8_t> load_embedded_base() {
    return std::vector<uint8_t>(base8k_rom, base8k_rom + base8k_rom_len);
}
static std::vector<uint8_t> load_embedded_loader() {
    return std::vector<uint8_t>(loader_bin, loader_bin + loader_bin_len);
}

static std::string basename_no_ext(const std::string& path) {
    auto slash = path.find_last_of("/\\");
    std::string name = (slash == std::string::npos) ? path : path.substr(slash + 1);
    auto dot = name.find_last_of('.');
    if (dot != std::string::npos) name.resize(dot);
    return name;
}

static std::string derive_output_name(const std::string& p_path) {
    return basename_no_ext(p_path) + ".rom";
}

static bool file_exists(const char* p) {
    struct stat st{};
    return p && *p && (stat(p, &st) == 0) && S_ISREG(st.st_mode);
}



int main(int argc, char** argv) {

    const char* base_path  = nullptr;
    const char* loader_path = nullptr;
    const char* out_path   = nullptr;

    int opt;
    while ((opt = getopt(argc, argv, "b:u:o:h")) != -1) {
        switch (opt) {
            case 'b': base_path  = optarg; break;
            case 'l': loader_path = optarg; break;
            case 'o': out_path   = optarg; break;
            case 'h':
            default:
                std::cerr <<
                  "Usage: " << argv[0] << " [-b base8k.rom] [-u loader.bin] [-o out.rom] <program.p|program.p.zx7>\n"
                  "  -b  Optional\n"
                  "  -l  Optional\n"
                  "  -o  Optional; default is <p-file>.rom\n";
                return (opt=='h') ? 0 : 1;
        }
    }
    
    if (optind >= argc) {
        std::cerr << "Error: no P-file (.p or .p.zx7)\n";
        return 1;
    }
    

    std::string p_path = argv[optind];
    std::string out_file = out_path ? std::string(out_path) : derive_output_name(p_path);

    try {
        // Base ROM
        std::vector<uint8_t> base = file_exists(base_path) ? slurp(base_path) : load_embedded_base();
        if (base.size() != 8192) throw std::runtime_error("Base ROM must be exactly 8K");

        // loader stub
        std::vector<uint8_t> stub = file_exists(loader_path) ? slurp(loader_path) : load_embedded_loader();
        if (stub.size() > 8192) throw std::runtime_error("Loader too large for upper 8K");

        // P-data (auto-komprimer hvis ikke .zx7)
        auto payload_raw = slurp(p_path);
        std::vector<uint8_t> pzx7;
        bool already_zx7 = is_zx7(p_path);
        if (already_zx7) {
            pzx7 = std::move(payload_raw);
            std::cout << "[info] Input is already ZX7-compressed\n";
        } else {
            std::cout << "[info] Compressing input with ZX7… ";
            pzx7 = zx7_encode(payload_raw);
            std::cout << "done (" << pzx7.size() << " bytes)\n";
        }

        // Byg 16K ROM
        std::vector<uint8_t> rom(16384, 0x00);

        // Nedre 8K
        std::copy(base.begin(), base.end(), rom.begin());

        // Øvre 8K: stub + data
        const size_t loader_off = 0x2000;
        size_t cursor = loader_off;
        if (cursor + stub.size() > rom.size()) throw std::runtime_error("Loader does not fit");
        std::copy(stub.begin(), stub.end(), rom.begin() + cursor);
        cursor += stub.size();

        if (cursor + pzx7.size() > rom.size())
            throw std::runtime_error("Compressed P file too large to fit in upper 8K");

        std::copy(pzx7.begin(), pzx7.end(), rom.begin() + cursor);
        cursor += pzx7.size();

        // Skriv ud
        std::ofstream out(out_file, std::ios::binary);
        out.write(reinterpret_cast<const char*>(rom.data()), (std::streamsize)rom.size());
        if (!out) throw std::runtime_error("Could not write output");

        // Resume
        size_t used_upper = stub.size() + pzx7.size();
        size_t free_upper = 8192 - used_upper;
        std::cout
          << "OK → " << out_file << "\n"
          << "  Base:  " << (base_path ? base_path : "[embedded]") << "  (" << base.size() << " bytes)\n"
          << "  Loader: " << (loader_path ? loader_path : "[embedded]") << " (" << stub.size() << " bytes)\n"
          << "  Payload (.zx7): " << pzx7.size()
          << " bytes" << (already_zx7 ? " (input was already .zx7)" :
                          (" (from " + std::to_string(payload_raw.size()) + " raw bytes)")) << "\n"
          << "  Upper-block: Used " << used_upper << " / 8192 bytes  (free " << free_upper << ")\n";

        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 2;
    }
}

