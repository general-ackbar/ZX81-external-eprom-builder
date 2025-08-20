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
#include "menuloader.h"  // New header for menu loader

extern "C" {
    #include "zx7/zx7.h"
}

struct CompressedPFile {
    std::string original_name;
    std::vector<uint8_t> compressed_data;
    size_t offset;  // Offset in ROM where this P-file is stored
};

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

static std::vector<uint8_t> load_embedded_menuloader() {
    return std::vector<uint8_t>(menuloader_bin, menuloader_bin + menuloader_bin_len);
}

static std::string basename_no_ext(const std::string& path) {
    auto slash = path.find_last_of("/\\");
    std::string name = (slash == std::string::npos) ? path : path.substr(slash + 1);
    auto dot = name.find_last_of('.');
    if (dot != std::string::npos) name.resize(dot);
    if (name.size() > 2 && name.compare(name.size()-2, 2, ".p") == 0) {
        name.resize(name.size()-2);
    }

    return name;
}

static std::string derive_output_name(const std::vector<std::string>& p_paths) {
    if (p_paths.size() == 1) {
        return basename_no_ext(p_paths[0]) + ".rom";
    } else {
        return "multi.rom";  // Default name for multi-file ROM
    }
}

static bool file_exists(const char* p) {
    struct stat st{};
    return p && *p && (stat(p, &st) == 0) && S_ISREG(st.st_mode);
}

static uint8_t ascii_to_zx81(char c) {
    // Numbers 0-9
    if (c >= '0' && c <= '9') return c - 20;          // ASCII 48-57 → ZX81 0x1C-0x25
    
    // Letters A-Z  
    if (c >= 'A' && c <= 'Z') return c - 27;          // ASCII 65-90 → ZX81 0x26-0x3F
    
    // Special characters
    switch (c) {
        case ' ':  return 0x00;  // ZX_SPACE
        case '"':  return 0x0B;  // ZX_QUOTE
        case '#':  return 0x0C;  // ZX_POUND
        case '$':  return 0x0D;  // ZX_DOLLAR
        case ':':  return 0x0E;  // ZX_COLON
        case '?':  return 0x0F;  // ZX_QUERY
        case '(':  return 0x10;  // ZX_BRACKET_LEFT
        case ')':  return 0x11;  // ZX_BRACKET_RIGHT
        case '>':  return 0x12;  // ZX_GREATER_THAN
        case '<':  return 0x13;  // ZX_LESS_THAN
        case '=':  return 0x14;  // ZX_EQUAL
        case '+':  return 0x15;  // ZX_PLUS
        case '-':  return 0x16;  // ZX_MINUS
        case '*':  return 0x17;  // ZX_STAR
        case '/':  return 0x18;  // ZX_SLASH
        case ';':  return 0x19;  // ZX_SEMICOLON
        case ',':  return 0x1A;  // ZX_COMMA
        case '.':  return 0x1B;  // ZX_PERIOD
        default:   return 0x00;  // Fallback to space for unknown chars
    }
}

int main(int argc, char** argv) {
    const char* base_path  = nullptr;
    const char* loader_path = nullptr;
    const char* out_path   = nullptr;
	bool use_simple_menu = false;
	
    int opt;
    while ((opt = getopt(argc, argv, "b:l:o:hs")) != -1) {
        switch (opt) {
            case 'b': base_path  = optarg; break;
            case 'l': loader_path = optarg; break;
            case 'o': out_path   = optarg; break;
            case 's': use_simple_menu = true; break;
            case 'h':
            default:
                std::cerr <<
                  "Usage: " << argv[0] << " [-b base8k.rom] [-l loader.bin] [-o out.rom] <program1.p> [program2.p] [...]\n"
                  "  -b  Optional base ROM (8K)\n"
                  "  -l  Optional loader (ignored when multiple P-files, uses menu loader)\n"
                  "  -o  Optional output name\n"
                  "  Multiple P-files will create a menu-driven ROM\n";
                return (opt=='h') ? 0 : 1;
        }
    }
    
    if (optind >= argc) {
        std::cerr << "Error: no P-file(s) specified\n";
        return 1;
    }

    // Collect all P-file paths
    std::vector<std::string> p_paths;
    for (int i = optind; i < argc; i++) {
        p_paths.push_back(argv[i]);
    }

    std::string out_file = out_path ? std::string(out_path) : derive_output_name(p_paths);

    try {
        // Base ROM
        std::vector<uint8_t> base = file_exists(base_path) ? slurp(base_path) : load_embedded_base();
        if (base.size() != 8192) throw std::runtime_error("Base ROM must be exactly 8K");

        // Choose loader based on number of P-files
        std::vector<uint8_t> stub;
        bool use_menu = (p_paths.size() > 1);
        
        if (use_menu) {
            stub = load_embedded_menuloader();
            std::cout << "[info] Using menu loader for " << p_paths.size() << " P-files\n";
        } else {
            stub = file_exists(loader_path) ? slurp(loader_path) : load_embedded_loader();
            std::cout << "[info] Using single-file loader\n";
        }
        
        if (stub.size() > 8192) throw std::runtime_error("Loader too large for upper 8K");

        // Process all P-files
        std::vector<CompressedPFile> compressed_files;
        size_t total_compressed_size = 0;

        for (const auto& p_path : p_paths) {
            CompressedPFile pfile;
            pfile.original_name = basename_no_ext(p_path);
            
            auto payload_raw = slurp(p_path);
            bool already_zx7 = is_zx7(p_path);
            
            if (already_zx7) {
                pfile.compressed_data = std::move(payload_raw);
                std::cout << "[info] " << pfile.original_name << " is already ZX7-compressed (" 
                         << pfile.compressed_data.size() << " bytes)\n";
            } else {
                std::cout << "[info] Compressing " << pfile.original_name << " with ZX7... ";
                pfile.compressed_data = zx7_encode(payload_raw);
                std::cout << "done (" << pfile.compressed_data.size() << " bytes, from " 
                         << payload_raw.size() << " raw)\n";
            }
            
            total_compressed_size += pfile.compressed_data.size();
            compressed_files.push_back(std::move(pfile));
        }

        // Check if everything fits (including filename block)
        size_t filename_block_size = 0;
        if (use_menu && compressed_files.size() > 1) {
            for (const auto& pfile : compressed_files) {
                std::string filename_entry = std::to_string(&pfile - &compressed_files[0] + 1) + ") " + pfile.original_name;
                filename_block_size += filename_entry.length();
            }
        }
        
        size_t available_space = 8192 - stub.size();
        size_t total_needed = filename_block_size + total_compressed_size;
        
        if (total_needed > available_space) {
            throw std::runtime_error("Filename block (" + std::to_string(filename_block_size) + 
                                   " bytes) + compressed P-files (" + std::to_string(total_compressed_size) + 
                                   " bytes) don't fit in available space (" + std::to_string(available_space) + " bytes)");
        }

        // Build 16K ROM
        std::vector<uint8_t> rom(16384, 0x00);

        // Lower 8K: base ROM
        std::copy(base.begin(), base.end(), rom.begin());

        // Upper 8K: loader + filenames + P-files
        const size_t loader_off = 0x2000;
        size_t cursor = loader_off;
        
        // Copy loader
        std::copy(stub.begin(), stub.end(), rom.begin() + cursor);
        cursor += stub.size();

        // Add filename block (only for multi-file mode)
        size_t filename_block_start = cursor;
        if (use_menu && compressed_files.size() > 1) {
            

        	if(use_simple_menu){
        		 std::cout << "[info] Writing simple menu\n";	
        		 rom[cursor++] =  ascii_to_zx81( std::to_string(compressed_files.size())[0] );
        		 rom[cursor++] = 0x09;            
        	} 
        	else 
        	{
	        	rom[cursor++] =  ascii_to_zx81( std::to_string(compressed_files.size())[0] );
	        	rom[cursor++] = 0x76;
	            std::cout << "[info] Writing filename block at offset 0x" << std::hex << cursor << std::dec << "\n";
            
	            for (size_t i = 0; i < compressed_files.size(); i++) {
           	     std::string filename_entry = std::to_string(i + 1) + ") " + compressed_files[i].original_name;
                
            	    // Uppercase
//					std::transform(filename_entry.begin(), filename_entry.end(), filename_entry.begin(), [](unsigned char c) { return std::toupper(c); });

        	        rom[cursor++] = 0x76;	//Start by adding a new line
    	            // Write each character as a byte
	                for (char c : filename_entry) {
        	            if (cursor >= rom.size()) {
    	                    throw std::runtime_error("Filename block exceeds ROM size");
	                    }
                    
            	        c = std::toupper(static_cast<unsigned char>(c));
					    rom[cursor++] = ascii_to_zx81(c);
    	            }
	                rom[cursor++] = 0x76;	//New line
                	std::cout << "[info]   Entry " << (i + 1) << ": \"" << filename_entry << "\" (" << filename_entry.length() << " bytes)\n";
            	}
				rom[cursor++] = 0x09; // Add String terminator
    	        size_t filename_block_size = cursor - filename_block_start;
	            std::cout << "[info] Filename block: " << filename_block_size << " bytes total\n";
            }
        }

        // Store P-files and track their offsets
        for (auto& pfile : compressed_files) {
            pfile.offset = cursor;
            std::copy(pfile.compressed_data.begin(), pfile.compressed_data.end(), rom.begin() + cursor);
            cursor += pfile.compressed_data.size();
            
            std::cout << "[info] " << pfile.original_name << " stored at offset 0x" 
                     << std::hex << pfile.offset << std::dec << "\n";
        }

        // Patch menu loader with actual P-file offsets (only for multi-file mode)
        if (use_menu && compressed_files.size() > 1) {
            std::cout << "[info] Patching menu loader with P-file offsets...\n";
            
            size_t search_start = loader_off;
            size_t search_end = loader_off + stub.size();
            size_t patches_made = 0;
            
            for (size_t i = search_start; i <= search_end - 3 && patches_made < compressed_files.size(); i++) {
                // Look for pattern: 21 00 20 (LD HL, $2000)
                if (rom[i] == 0x21 && rom[i+1] == 0x00 && rom[i+2] == 0x20) {
                    uint16_t offset = static_cast<uint16_t>(compressed_files[patches_made].offset);
                    uint8_t low_byte = offset & 0xFF;
                    uint8_t high_byte = (offset >> 8) & 0xFF;
                    
                    rom[i+1] = low_byte;
                    rom[i+2] = high_byte;
                    
                    std::cout << "[info]   Patch " << patches_made << ": 0x" << std::hex << (i - loader_off) 
                             << " -> LD HL,$" << std::hex << offset << std::dec 
                             << " (" << compressed_files[patches_made].original_name << ")\n";
                    
                    patches_made++;
                }
            }
            
            if (patches_made != compressed_files.size()) {
                std::cout << "[warning] Expected " << compressed_files.size() 
                         << " patches but made " << patches_made << "\n";
            }
        }

        // Write ROM
        std::ofstream out(out_file, std::ios::binary);
        out.write(reinterpret_cast<const char*>(rom.data()), (std::streamsize)rom.size());
        if (!out) throw std::runtime_error("Could not write output");

        // Summary
        size_t used_upper = stub.size() + filename_block_size + total_compressed_size;
        size_t free_upper = 8192 - used_upper;
        
        std::cout << "OK → " << out_file << "\n"
                 << "  Base:  " << (base_path ? base_path : "[embedded]") << "  (" << base.size() << " bytes)\n"
                 << "  Loader: " << (use_menu ? "[embedded menu]" : (loader_path ? loader_path : "[embedded single]")) 
                 << " (" << stub.size() << " bytes)\n";
        
        if (use_menu && filename_block_size > 0) {
            std::cout << "  Filenames: " << filename_block_size << " bytes\n";
        }
        
        std::cout << "  P-files: " << compressed_files.size() << " files, " << total_compressed_size << " bytes total\n"
                 << "  Upper-block: Used " << used_upper << " / 8192 bytes  (free " << free_upper << ")\n";

        if (use_menu) {
            std::cout << "\nP-file offsets for menu loader:\n";
            for (const auto& pfile : compressed_files) {
                std::cout << "  " << pfile.original_name << ": 0x" << std::hex << pfile.offset << std::dec << "\n";
            }
        }

        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 2;
    }
}