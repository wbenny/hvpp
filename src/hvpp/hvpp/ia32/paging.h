#pragma once
#include <cstdint>
#include <type_traits>

namespace ia32 {

//
// Page Map Level
//
enum class pml : uint8_t
{
  //
  // Page Table
  //
  pt   = 0,

  //
  // Page Directory
  //
  pd   = 1,

  //
  // Page Directory Pointer Table
  //
  pdpt = 2,

  //
  // Page Map Level 4
  //
  pml4 = 3,
};

constexpr inline pml& operator++(pml& ptl) noexcept
{ ((uint8_t&)(ptl))++; return ptl; }

constexpr inline pml& operator--(pml& ptl) noexcept
{ ((uint8_t&)(ptl))--; return ptl; }

constexpr inline pml operator++(pml& ptl, int) noexcept
{ auto result = ptl; ((uint8_t&)(ptl))++; return result; }

constexpr inline pml operator--(pml& ptl, int) noexcept
{ auto result = ptl; ((uint8_t&)(ptl))--; return result; }

constexpr inline pml operator+(pml ptl, uint8_t value) noexcept
{ return static_cast<pml>(static_cast<uint8_t>(ptl) + value); }

constexpr inline pml operator-(pml ptl, uint8_t value) noexcept
{ return static_cast<pml>(static_cast<uint8_t>(ptl) - value); }

constexpr inline pml& operator+=(pml& ptl, uint8_t value) noexcept
{ ((uint8_t&)(ptl)) += value; return ptl; }

constexpr inline pml& operator-=(pml& ptl, uint8_t value) noexcept
{ ((uint8_t&)(ptl)) -= value; return ptl; }

//
// Page Table Entry
//
struct pte_t
{
  union
  {
    uint64_t flags;

    struct
    {
      uint64_t present : 1;
      uint64_t write : 1;
      uint64_t supervisor : 1;
      uint64_t page_level_write_through : 1;
      uint64_t page_level_cache_disable : 1;
      uint64_t accessed : 1;
      uint64_t dirty : 1;
      uint64_t pat : 1;
      uint64_t global : 1;
      uint64_t ignored_1 : 3;
      uint64_t page_frame_number : 36;
      uint64_t reserved1 : 4;
      uint64_t ignored_2 : 7;
      uint64_t protection_key : 4;
      uint64_t execute_disable : 1;
    };
  };
};

//
// Page Directory Entry
//
struct pde_t
{
  union
  {
    uint64_t flags;

    struct
    {
      uint64_t present : 1;
      uint64_t write : 1;
      uint64_t supervisor : 1;
      uint64_t page_level_write_through : 1;
      uint64_t page_level_cache_disable : 1;
      uint64_t accessed : 1;
      uint64_t reserved1 : 1;
      uint64_t large_page : 1;
      uint64_t ignored_1 : 4;
      uint64_t page_frame_number : 36;
      uint64_t reserved2 : 4;
      uint64_t ignored_2 : 11;
      uint64_t execute_disable : 1;
    };
  };
};

//
// Large Page Directory Entry
//
struct pde_large_t
{
  union
  {
    uint64_t flags;

    struct
    {
      uint64_t present : 1;
      uint64_t write : 1;
      uint64_t supervisor : 1;
      uint64_t page_level_write_through : 1;
      uint64_t page_level_cache_disable : 1;
      uint64_t accessed : 1;
      uint64_t dirty : 1;
      uint64_t large_page : 1;
      uint64_t global : 1;
      uint64_t ignored_1 : 3;
      uint64_t pat : 1;
      uint64_t reserved1 : 17;
      uint64_t page_frame_number : 18;
      uint64_t reserved2 : 4;
      uint64_t ignored_2 : 7;
      uint64_t protection_key : 4;
      uint64_t execute_disable : 1;
    };
  };
};

//
// Page Directory Pointer Table Entry
//
struct pdpte_t
{
  union
  {
    uint64_t flags;

    struct
    {
      uint64_t present : 1;
      uint64_t write : 1;
      uint64_t supervisor : 1;
      uint64_t page_level_write_through : 1;
      uint64_t page_level_cache_disable : 1;
      uint64_t accessed : 1;
      uint64_t reserved1 : 1;
      uint64_t large_page : 1;
      uint64_t ignored_1 : 4;
      uint64_t page_frame_number : 36;
      uint64_t reserved2 : 4;
      uint64_t ignored_2 : 11;
      uint64_t execute_disable : 1;
    };
  };
};

//
// Large Page Directory Pointer Table Entry
//
struct pdpte_large_t
{
  union
  {
    uint64_t flags;

    struct
    {
      uint64_t present : 1;
      uint64_t write : 1;
      uint64_t supervisor : 1;
      uint64_t page_level_write_through : 1;
      uint64_t page_level_cache_disable : 1;
      uint64_t accessed : 1;
      uint64_t dirty : 1;
      uint64_t large_page : 1;
      uint64_t global : 1;
      uint64_t ignored_1 : 3;
      uint64_t pat : 1;
      uint64_t reserved1 : 17;
      uint64_t page_frame_number : 18;
      uint64_t reserved2 : 4;
      uint64_t ignored_2 : 7;
      uint64_t protection_key : 4;
      uint64_t execute_disable : 1;
    };
  };
};

//
// Page Map Level 4 Entry
//
struct pml4e_t
{
  union
  {
    uint64_t flags;

    struct
    {
      uint64_t present : 1;
      uint64_t write : 1;
      uint64_t supervisor : 1;
      uint64_t page_level_write_through : 1;
      uint64_t page_level_cache_disable : 1;
      uint64_t accessed : 1;
      uint64_t reserved1 : 1;
      uint64_t must_be_zero : 1;
      uint64_t ignored_1 : 4;
      uint64_t page_frame_number : 36;
      uint64_t reserved2 : 4;
      uint64_t ignored_2 : 11;
      uint64_t execute_disable : 1;
    };
  };
};

struct page_descriptor_tag{};

//
// Page Table type descriptor
//
struct pt_t
{
  using        descriptor_tag = page_descriptor_tag;
  using                 entry = pte_t;
  static constexpr auto level = pml::pt;
  static constexpr auto count = uint64_t(512);
  static constexpr auto shift = uint64_t(12);           // 12 = (first set bit of 4kb) - 1
  static constexpr auto size  = uint64_t(1) << shift;   // 4kb
  static constexpr auto mask = ~(size - 1);
};

//
// Page Directory type descriptor
//
struct pd_t
{
  using        descriptor_tag = page_descriptor_tag;
  using                 entry = pde_t;
  static constexpr auto level = pml::pd;
  static constexpr auto count = uint64_t(512);
  static constexpr auto shift = pt_t::shift + 9;        // 21
  static constexpr auto size  = uint64_t(1) << shift;   // 2MB
  static constexpr auto mask = ~(size - 1);
};

//
// Page Directory Pointer Table type descriptor
//
struct pdpt_t
{
  using        descriptor_tag = page_descriptor_tag;
  using                 entry = pdpte_t;
  static constexpr auto level = pml::pdpt;
  static constexpr auto count = uint64_t(512);
  static constexpr auto shift = pd_t::shift + 9;        // 30
  static constexpr auto size  = uint64_t(1) << shift;   // 1GB
  static constexpr auto mask = ~(size - 1);
};

//
// Page Map Level 4 type descriptor
//
struct pml4_t
{
  using        descriptor_tag = page_descriptor_tag;
  using                 entry = pml4e_t;
  static constexpr auto level = pml::pml4;
  static constexpr auto count = uint64_t(512);
  static constexpr auto shift = pdpt_t::shift + 9;      // 39
  static constexpr auto size  = uint64_t(1) << shift;   // 512GB
  static constexpr auto mask  = ~(size - 1);
};

//
// Page entry and common fields
//
struct pe_t
{
  union
  {
    uint64_t flags;

    pml4e_t       pml4;
    pdpte_large_t pdpt_large; // 1GB
    pdpte_t       pdpt;
    pde_large_t   pd_large;   // 2MB
    pde_t         pd;
    pte_t         pt;

    //
    // Common fields.
    //

    struct
    {
      uint64_t present : 1;
      uint64_t write : 1;
      uint64_t supervisor : 1;
      uint64_t page_level_write_through : 1;
      uint64_t page_level_cache_disable : 1;
      uint64_t accessed : 1;
      uint64_t dirty : 1;
      uint64_t large_page : 1;
      uint64_t global : 1;
      uint64_t ignored_1 : 3;
      uint64_t page_frame_number : 36;
      uint64_t reserved1 : 4;
      uint64_t ignored_2 : 7;
      uint64_t protection_key : 4;
      uint64_t execute_disable : 1;
    };
  };
};

static_assert(sizeof(pe_t) == 8);

//
// Align provided value down to the specified paging structure boundary.
//
template <
  typename T,
  typename PAGE_DESCRIPTOR,
  typename = std::enable_if_t<
    std::is_base_of_v<page_descriptor_tag, typename PAGE_DESCRIPTOR::descriptor_tag> && (
      std::is_pointer_v<T> ||
     (std::is_integral_v<T> && sizeof(T) == sizeof(uintptr_t))
    )
  >
>
constexpr inline T page_align(T va, PAGE_DESCRIPTOR) noexcept
{ return (T)(uintptr_t(va) & PAGE_DESCRIPTOR::mask); }

//
// Align provided value up to the specified paging structure boundary.
//
template <
  typename T,
  typename PAGE_DESCRIPTOR,
  typename = std::enable_if_t<
    std::is_base_of_v<page_descriptor_tag, typename PAGE_DESCRIPTOR::descriptor_tag> && (
      std::is_pointer_v<T> ||
     (std::is_integral_v<T> && sizeof(T) == sizeof(uintptr_t))
    )
  >
>
constexpr inline T page_align_up(T va, PAGE_DESCRIPTOR) noexcept
{ return (T)((uintptr_t(va) + PAGE_DESCRIPTOR::size - 1) & PAGE_DESCRIPTOR::mask); }

//
// Return offset of the page.
//
template <
  typename T,
  typename PAGE_DESCRIPTOR,
  typename = std::enable_if_t<
    std::is_base_of_v<page_descriptor_tag, typename PAGE_DESCRIPTOR::descriptor_tag> && (
      std::is_pointer_v<T> ||
     (std::is_integral_v<T> && sizeof(T) == sizeof(uintptr_t))
    )
  >
>
constexpr inline uint64_t byte_offset(T va, PAGE_DESCRIPTOR) noexcept
{ return (uint64_t)(uintptr_t(va) & ~PAGE_DESCRIPTOR::mask); }

//
// Return how many pages are needed to cover specified size.
//
template <
  typename T,
  typename PAGE_DESCRIPTOR,
  typename = std::enable_if_t<
    std::is_base_of_v<page_descriptor_tag, typename PAGE_DESCRIPTOR::descriptor_tag> &&
    std::is_integral_v<T>
  >
>
constexpr inline uint64_t bytes_to_pages(T size, PAGE_DESCRIPTOR) noexcept
{ return (size >> PAGE_DESCRIPTOR::shift) + ((size & ~PAGE_DESCRIPTOR::mask) != 0); }

//
// Round specified value up to the page boundary.
//
template <
  typename T,
  typename PAGE_DESCRIPTOR,
  typename = std::enable_if_t<
    std::is_base_of_v<page_descriptor_tag, typename PAGE_DESCRIPTOR::descriptor_tag> &&
    std::is_integral_v<T>
  >
>
constexpr inline uint64_t round_to_pages(T size, PAGE_DESCRIPTOR) noexcept
{ return uint64_t(page_align_up(size, PAGE_DESCRIPTOR{})); }

//
// Helper functions for 4kb pages.
//

template <typename T>
constexpr inline T page_align(T va) noexcept
{ return page_align(va, pt_t{}); }

template <typename T>
constexpr inline T page_align_up(T va) noexcept
{ return page_align_up(va, pt_t{}); }

template <typename T>
constexpr inline uint64_t byte_offset(T va) noexcept
{ return byte_offset(va, pt_t{}); }

template <typename T>
constexpr inline uint64_t bytes_to_pages(T size) noexcept
{ return bytes_to_pages(size, pt_t{}); }

template <typename T>
constexpr inline uint64_t round_to_pages(T size) noexcept
{ return round_to_pages(size, pt_t{}); }

}
