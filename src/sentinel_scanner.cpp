#include "core/sentinel_scanner.h"
#include <string>
SentinelScanner::SentinelScanner(std::string sentinel)  //Constructor
    : sentinel_(std::move(sentinel)), pending_(""){
}

SentinelScanner::Out SentinelScanner::feed(std::string_view chunk){
    pending_.append(chunk);
    Out result{"", false};
    std::size_t pos = pending_.find(sentinel_);
    if (pos != std::string::npos){
        result.safe_text = pending_.substr(0, pos);
        result.sentinel_found = true;
        pending_.clear();
        return result;
    }
    // No complete sentinel found.
    // Keep enough characters to detect a sentinel split across chunks.
    std::size_t keep = sentinel_.size() > 0 ? sentinel_.size() - 1: 0;
    if (pending_.size() > keep){
        std::size_t safe_count = pending_.size() - keep;
        result.safe_text = pending_.substr(0, safe_count);
        pending_.erase(0, safe_count);
    }
    return result;
}
SentinelScanner::Out SentinelScanner::flush(){
    Out result{pending_, false};
    pending_.clear();
    return result;
}