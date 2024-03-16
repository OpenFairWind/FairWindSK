//
// Created by Raffaele Montella on 16/03/24.
//

#ifndef ITERABLELAYOUTADAPTER_HPP
#define ITERABLELAYOUTADAPTER_HPP

#include <QLayout>
#include <QDebug>
#include <QPointer>
#include <utility>

template<class WT> class IterableLayoutAdapter;

template<typename WT>
class LayoutIterator {
    QPointer<QLayout> m_layout;
    int m_index;
    friend class IterableLayoutAdapter<WT>;
    LayoutIterator(QLayout * layout, int dir) :
            m_layout(layout), m_index(dir>0 ? -1 : m_layout->count()) {
        if (dir > 0) ++*this;
    }
    friend QDebug operator<<(QDebug dbg, const LayoutIterator & it) {
        return dbg << it.m_layout << it.m_index;
    }
    friend void swap(LayoutIterator& a, LayoutIterator& b) {
        using std::swap;
        swap(a.m_layout, b.m_layout);
        swap(a.m_index, b.m_index);
    }
public:
    LayoutIterator() : m_index(0) {}
    LayoutIterator(const LayoutIterator & o) :
            m_layout(o.m_layout), m_index(o.m_index) {}
    LayoutIterator(LayoutIterator && o) { swap(*this, o); }
    LayoutIterator & operator=(LayoutIterator o) {
        swap(*this, o);
        return *this;
    }
    WT * operator*() const { return static_cast<WT*>(m_layout->itemAt(m_index)->widget()); }
    const LayoutIterator & operator++() {
        while (++m_index < m_layout->count() && !qobject_cast<WT*>(m_layout->itemAt(m_index)->widget()));
        return *this;
    }
    LayoutIterator operator++(int) {
        LayoutIterator temp(*this);
        ++*this;
        return temp;
    }
    const LayoutIterator & operator--() {
        while (!qobject_cast<WT*>(m_layout->itemAt(--m_index)->widget()) && m_index > 0);
        return *this;
    }
    LayoutIterator operator--(int) {
        LayoutIterator temp(*this);
        --*this;
        return temp;
    }
    bool operator==(const LayoutIterator & o) const { return m_index == o.m_index; }
    bool operator!=(const LayoutIterator & o) const { return m_index != o.m_index; }
};

template <class WT = QWidget>
class IterableLayoutAdapter {
    QPointer<QLayout> m_layout;
public:
    typedef LayoutIterator<WT> const_iterator;
    IterableLayoutAdapter(QLayout * layout) : m_layout(layout) {}
    const_iterator begin() const { return const_iterator(m_layout, 1); }
    const_iterator end() const { return const_iterator(m_layout, -1); }
    const_iterator cbegin() const { return const_iterator(m_layout, 1); }
    const_iterator cend() const { return const_iterator(m_layout, -1); }
};

template <class WT = QWidget>
class ConstIterableLayoutAdapter : public IterableLayoutAdapter<const WT> {
public:
    ConstIterableLayoutAdapter(QLayout * layout) : IterableLayoutAdapter<const WT>(layout) {}
};

#endif //ITERABLELAYOUTADAPTER_HPP
