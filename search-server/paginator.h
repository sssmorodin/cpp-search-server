#pragma once
#include <iostream>
#include <vector>

template <typename Iterator>
class IteratorRange {
public:
    IteratorRange(Iterator& range_begin, Iterator& range_end)
            : begin_(range_begin)
            , end_(range_end)
            , size_(distance(range_begin, range_end)) {
    }

    auto begin() const {
        return begin_;
    }

    auto end() const {
        return end_;
    }

    size_t size() {
        return size_;
    }

private:
    Iterator begin_, end_;
    size_t size_;
};

template <typename Iterator>
class Paginator {
public:
    Paginator(Iterator documents_begin, Iterator documents_end, int page_size) {
        while(distance(documents_begin, documents_end) > page_size) {
            auto it_current = documents_begin;
            advance(documents_begin, page_size);
            pages_.push_back(IteratorRange(it_current, documents_begin));
        }
        pages_.push_back(IteratorRange(documents_begin, documents_end));
    }

    auto begin() const {
        return pages_.begin();
    }

    auto end() const {
        return pages_.end();
    }

private:
    std::vector<IteratorRange<Iterator>> pages_;
};

template <typename Container>
auto Paginate(const Container& c, size_t page_size) {
    return Paginator(begin(c), end(c), page_size);
}

template <typename Iterator>
std::ostream& operator<<(std::ostream& output, const IteratorRange<Iterator>& page) {
    for (auto it = page.begin(); it != page.end(); ++it) {
        output << *it;
    }
    return output;
}