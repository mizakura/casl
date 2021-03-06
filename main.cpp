#include <iostream>
#include <iomanip>
#include <fstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <map>
#include <sstream>
#include <set>
#include <algorithm>

std::unordered_map<std::string, uint16_t> NimonicMap {
  {"NOP", 0x0000},
  {"LD", 0x1000},
  {"ST", 0x1100},
  {"LAD", 0x1200},

  {"ADDA", 0x2000},
  {"SUBA", 0x2100},
  {"ADDL", 0x2200},
  {"SUBL", 0x2300},

  {"AND", 0x3000},
  {"OR", 0x3100},
  {"XOR", 0x3200},

  {"CPA", 0x4000},
  {"CPL", 0x4100},
  
  {"SLA", 0x5000},
  {"SRA", 0x5100},
  {"SLL", 0x5200},
  {"SRL", 0x5300},
  {"JMI", 0x6100},
  {"JNZ", 0x6200},
  {"JZE", 0x6300},
  {"JUMP", 0x6400},
  {"JPL", 0x6500},
  {"JOV", 0x6600},

  {"PUSH", 0x7000},
  {"POP", 0x7100},

  {"CALL", 0x8000},
  {"RET", 0x8100},

  {"SVC", 0xF000},

  {"START", 0xf1},
  {"END", 0xf2},
  {"DC", 0xf3},
  {"DS", 0xf4}
};

std::unordered_map<std::string, uint8_t> RegisterMap {
  {"GR0", 0x0},
  {"GR1", 0x1},
  {"GR2", 0x2},
  {"GR3", 0x3},
  {"GR4", 0x4},
  {"GR5", 0x5},
  {"GR6", 0x6},
  {"GR7", 0x7}
};

std::set<std::string> oneLenOp {
  "POP", "RET", "DC" 
};

std::set<std::string> jumpOp {
  "JMI", 
  "JNZ", 
  "JZE", 
  "JUMP",
  "JPL", 
  "JOV",
};

std::set<std::string> multiOp {
  "LD", 
  "ADDA", 
  "SUBA", 
  "ADDL", 
  "SUBL",
  "AND",
  "OR", 
  "XOR",
  "CPA", 
  "CPL"
};


std::unordered_map<std::string, uint16_t> LavelMap;

uint16_t ad2u16(std::string &address) {
  int radix = 10;
  if (address[0] == '#') {
    address.erase(0, 1);
    radix = 16;
  }
  try {
    return std::stoi(address.c_str(), nullptr, radix);
  }
  catch (std::invalid_argument&) {
    // std::cerr << "Address notation is invalid" << std::endl;
    throw -1; //enumとかでやったほうがよさそう
  }
}

uint8_t getOpLength(std::string nimonic, std::istringstream &ss) {
  uint8_t opLength = 0x1;
  if (!oneLenOp.count(nimonic)) {

     // DS用
    if (nimonic == "DS") {
      std::string operand;
      ss >> operand;
      try {
        opLength = ad2u16(operand);
      }
      catch (int errorcode) {
        std::cerr << "Address notation is invalid" << std::endl;
        return 0;
      }
    }

    // 命令長を複数もつ命令用
    while (!ss.eof()) {
      std::string operand;
      ss >> operand;
      if (!RegisterMap.count(operand)) {
        opLength = 0x2;
        break;
      }
    }
  }
  return opLength;
}

union U16BUF {
  uint16_t u;
  char c[2];
};


int main(int argc, char *argv[]) {
  std::ifstream ifs(argv[1]);
  if (ifs.fail()) {
    std::cerr << "Can't open "<< argv[1] << '.' << std::endl;
    return -1;
  }
  std::string assemblySrc((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
  std::cout << assemblySrc << std::endl;
    for (auto pos = assemblySrc.find(","); pos != std::string::npos; pos = assemblySrc.find(","))
      assemblySrc.replace(pos, 1, " ");

  std::istringstream assemblyss(assemblySrc);

  uint16_t address = 0x00;
  // first line, expect START nimonic
  std::string first_line;
  std::getline(assemblyss, first_line); 
  std::istringstream flss(first_line);

  //to get leading address
  std::string s_label, s_start, s_address;
  flss >> s_label >> s_start >> s_address;
  // assemblySrc.replace(0, assemblyss.tellg(), " ");
  if (s_start != "START") {
    std::cerr << "need START" << std::endl;
  }
  try {
    address = ad2u16(s_address);
  }
  catch (int errorcode) {
    std::cerr << "Address notation is invalid" << std::endl;
    std::cerr << first_line << std::endl;
    return 0;
  }

  while(!assemblyss.eof()) {
    // Label を登録していく
    std::string current_line;
    auto astellg = assemblyss.tellg();
    std::getline(assemblyss, current_line);
    std::istringstream clss(current_line);
    std::string token;
    clss >> token;

    if (token == "END") break;
      std::cout << "#" << std::setfill('0') << std::setw(4) << std::hex << static_cast<int>(address) << ' ' << current_line << std::endl;
    if (!NimonicMap.count(token)) {
      assemblySrc.replace(astellg, clss.tellg(), " ");
      LavelMap.insert(std::make_pair(token, address));
      clss >> token;
    }
    address += getOpLength(token, clss);
  }
  
  //std::cout << assemblySrc << std::endl;

  for (auto x : LavelMap)
    std::cout << x.first << ' ' << "#" << std::setfill('0') << std::setw(4) << std::hex << static_cast<int>(x.second) << std::endl;
  
  std::cout << std::endl;
  assemblyss.seekg(0, std::ios_base::beg);
  std::ofstream ofs(argv[2], std::ios::out|std::ios::binary);
  U16BUF writebuf;
  if (ofs.fail()) {
    std::cerr << "Can't open "<< argv[2] << '.' << std::endl;
    return -1;
  }

  std::string tmp;
  std::getline(assemblyss, tmp);
  while(!assemblyss.eof()) {
    std::string current_line;
    std::getline(assemblyss, current_line);
    std::istringstream clss(current_line);
    std::string op, r, adr, x;
    std::string token;
    clss >> op;
    if (op == "END") break;
    if (!NimonicMap.count(op)) {
      clss >> op;
    }
    while (!clss.eof()) {
      clss >> token;
      if (RegisterMap.count(token)) {
        r = token;
        if (clss.eof()) break;
        clss >> token;
      }
      if (LavelMap.count(token)) {
        adr = token;
        if (clss.eof()) break;
        clss >> token;
      }
      else {
        try {
          ad2u16(token);
          adr = token;
          if (clss.eof()) break;
          clss >> token;
        }
        catch (int error) {
        }
      }
      if (RegisterMap.count(token)) {
        x = token;
      }
    }

    std::cout << op << ' ' << r << ' ' << adr << ' ' << x << std::endl;

    if (op == "DS") {
      try {
        for (int i = 0; i < static_cast<int>(ad2u16(adr)); ++i) {
          std::cout << "#" << std::setfill('0') << std::setw(4) << std::hex << static_cast<int>(static_cast<uint16_t>(0x0)) << std::endl;
          writebuf.u = static_cast<uint16_t>(0x0);
          ofs.write(writebuf.c, sizeof(unsigned char) *2);
        }
        continue;
      }
      catch(int error) {return -1;}
    }

    if (op == "DC") {
      try {
          std::cout << "#" << std::setfill('0') << std::setw(4) << std::hex << static_cast<int>(ad2u16(adr)) << std::endl;
          writebuf.u = ad2u16(adr);
          ofs.write(writebuf.c, sizeof(unsigned char) *2);
          continue;
      }
      catch (int error) {return -1;}
    }
    uint16_t output1 = NimonicMap.at(op);
    if (RegisterMap.count(r))
      output1 += RegisterMap.at(r) << 4;
    if (RegisterMap.count(x))
      output1 += RegisterMap.at(x);
    uint16_t output2 = 0x0;
      if(LavelMap.count(adr))
        output2 = LavelMap.at(adr);
      else {
        try {
          output2 = ad2u16(adr);
        }
        catch (int error) {}
      }

      if (adr.empty() && multiOp.count(op)) {
        output1 += 0x0400;
      }
    
      if (oneLenOp.count(op) || adr.empty()) {
        std::cout << "#" << std::setfill('0') << std::setw(4) << std::hex << static_cast<int>(output1) << std::endl;
        writebuf.u = output1;
        ofs.write(writebuf.c, sizeof(unsigned char) *2);
      }

      else {
        std::cout << "#" << std::setfill('0') << std::setw(4) << std::hex << static_cast<int>(output1) << " #" << std::setfill('0') << std::setw(4) << std::hex << static_cast<int>(output2) << std::endl;
        writebuf.u = output1;
        ofs.write(writebuf.c, sizeof(unsigned char) *2);
        writebuf.u = output2;
        ofs.write(writebuf.c, sizeof(unsigned char) *2);
      }

  }
}
