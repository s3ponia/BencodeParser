# BencodeParser
## C++17 header-only library for parsing BencodeParser. 
### API
Bencode types are represented in aliases BencodeInteger, BencodeString, BencodeList, BencodeDictionary. BencodeElement is a struct with the only field ```std::variant<BencodeInteger, BencodeString, BencodeList, BencodeDictionary> value```.
#### For Decoding Bencode use 
```auto DecodeBencodeElement(std::string_view str) -> BencodeElement```
#### For Encoding Bencode use
```auto Encode(BencodeElement const &) -> std::string;```
