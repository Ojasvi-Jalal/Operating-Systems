/*
 * Buddy Page Allocation Algorithm
 * SKELETON IMPLEMENTATION -- TO BE FILLED IN FOR TASK (2)
 */

/*
 * STUDENT NUMBER: s1612970
*/
#include <infos/mm/page-allocator.h>
#include <infos/mm/mm.h>
#include <infos/kernel/kernel.h>
#include <infos/kernel/log.h>
#include <infos/util/math.h>
#include <infos/util/printf.h>

using namespace infos::kernel;
using namespace infos::mm;
using namespace infos::util;

#define MAX_ORDER	17

/**
 * A buddy page allocation algorithm.
 */
class BuddyPageAllocator : public PageAllocatorAlgorithm
{
private:
	/**
	 * Returns the number of pages that comprise a 'block', in a given order.
	 * @param order The order to base the calculation off of.
	 * @return Returns the number of pages in a block, in the order.
	 */
	static inline constexpr uint64_t pages_per_block(int order)
	{
		/* The number of pages per block in a given order is simply 1, shifted left by the order number.
		 * For example, in order-2, there are (1 << 2) == 4 pages in each block.
		 */
		return (1 << order);
	}

	/**
	 * Returns TRUE if the supplied page descriptor is correctly aligned for the
	 * given order.  Returns FALSE otherwise.
	 * @param pgd The page descriptor to test alignment for.
	 * @param order The order to use for calculations.
	 */
	static inline bool is_correct_alignment_for_order(const PageDescriptor *pgd, int order)
	{
		// Calculate the page-frame-number for the page descriptor, and return TRUE if
		// it divides evenly into the number pages in a block of the given order.
		return (sys.mm().pgalloc().pgd_to_pfn(pgd) % pages_per_block(order)) == 0;
	}

	/** Given a page descriptor, and an order, returns the buddy PGD.  The buddy could either be
	 * to the left or the right of PGD, in the given order.
	 * @param pgd The page descriptor to find the buddy for.
	 * @param order The order in which the page descriptor lives.
	 * @return Returns the buddy of the given page descriptor, in the given order.
	 */
	PageDescriptor *buddy_of(PageDescriptor *pgd, int order)
	{
		// (1) Make sure 'order' is within range
		if (order >= MAX_ORDER) {
			return NULL;
		}

		// (2) Check to make sure that PGD is correctly aligned in the order
		if (!is_correct_alignment_for_order(pgd, order)) {
			return NULL;
		}

		// (3) Calculate the page-frame-number of the buddy of this page.
		// * If the PFN is aligned to the next order, then the buddy is the next block in THIS order.
		// * If it's not aligned, then the buddy must be the previous block in THIS order.
		uint64_t buddy_pfn = is_correct_alignment_for_order(pgd, order + 1) ?
			sys.mm().pgalloc().pgd_to_pfn(pgd) + pages_per_block(order) :
			sys.mm().pgalloc().pgd_to_pfn(pgd) - pages_per_block(order);

		// (4) Return the page descriptor associated with the buddy page-frame-number.
		return sys.mm().pgalloc().pfn_to_pgd(buddy_pfn);
	}

	/**
	 * Inserts a block into the free list of the given order.  The block is inserted in ascending order.
	 * @param pgd The page descriptor of the block to insert.
	 * @param order The order in which to insert the block.
	 * @return Returns the slot (i.e. a pointer to the pointer that points to the block) that the block
	 * was inserted into.
	 */
	PageDescriptor **insert_block(PageDescriptor *pgd, int order)
	{
		// Starting from the _free_area array, find the slot in which the page descriptor
		// should be inserted.
		//mm_log.messagef(LogLevel::DEBUG, "INSERT_BLOCK: inserting at %d",order);
		PageDescriptor **slot = &_free_areas[order];

		// Iterate whilst there is a slot, and whilst the page descriptor pointer is numerically
		// greater than what the slot is pointing to.
		while (*slot && pgd > *slot) {
			slot = &(*slot)->next_free;
		}

		// Insert the page descriptor into the linked list.
		pgd->next_free = *slot;
		*slot = pgd;

		//mm_log.messagef(LogLevel::DEBUG, "INSERT_BLOCK: have inserted now at %d, here's the new state",order);
		dump_state();
		// Return the insert point (i.e. slot)
		return slot;
	}

	/**
	 * Removes a block from the free list of the given order.  The block MUST be present in the free-list, otherwise
	 * the system will panic.
	 * @param pgd The page descriptor of the block to remove.
	 * @param order The order in which to remove the block from.
	 */
	void remove_block(PageDescriptor *pgd, int order)
	{
		// Starting from the _free_area array, iterate until the block has been located in the linked-list.
		PageDescriptor **slot = &_free_areas[order];
		while (*slot && pgd != *slot) {
			slot = &(*slot)->next_free;
		}

		// Make sure the block actually exists.  Panic the system if it does not.
		assert(*slot == pgd);

		// Remove the block from the free list.
		*slot = pgd->next_free;
		pgd->next_free = NULL;
		//mm_log.messagef(LogLevel::DEBUG, "REMOVE_PAGES: Dump after removal");
		dump_state();
	}

	/**
	 * Given a pointer to a block of free memory in the order "source_order", this function will
	 * split the block in half, and insert it into the order below.
	 * @param block_pointer A pointer to a pointer containing the beginning of a block of free memory.
	 * @param source_order The order in which the block of free memory exists.  Naturally,
	 * the split will insert the two new blocks into the order below.
	 * @return Returns the left-hand-side of the new block.
	 */
	PageDescriptor *split_block(PageDescriptor **block_pointer, int source_order)
	{
		// Make sure there is an incoming pointer.
		assert(*block_pointer);

		// Make sure the block_pointer is correctly aligned.
		assert(is_correct_alignment_for_order(*block_pointer, source_order));

		//mm_log.messagef(LogLevel::DEBUG, "SPLIT_BLOCK: checking if source_order is 0");
		if(source_order == 0)
			return *block_pointer;
		else{
				//mm_log.messagef(LogLevel::DEBUG, "SPLIT_BLOCK: source order is not 0,so starting splitting");
				int split_order = source_order-1;
				uint64_t size = pages_per_block(split_order);

				//mm_log.messagef(LogLevel::DEBUG, "SPLIT_BLOCK: source_order:%d",source_order);
				//mm_log.messagef(LogLevel::DEBUG, "SPLIT_BLOCK: split_order:%d",split_order);

				PageDescriptor *first_half = *block_pointer;
				PageDescriptor *second_half = first_half+size;

				//mm_log.messagef(LogLevel::DEBUG, "SPLIT_BLOCK: removing the block now");
				assert(*block_pointer == first_half);
				//mm_log.messagef(LogLevel::DEBUG, "SPLIT_BLOCK: I looked for %lx, I found %lx", sys.mm().pgalloc().pgd_to_pfn(*block_pointer),sys.mm().pgalloc().pgd_to_pfn(first_half));
				remove_block(first_half, source_order);
				//mm_log.messagef(LogLevel::DEBUG, "SPLIT_BLOCK: dump_state after removing");
				dump_state();
				//mm_log.messagef(LogLevel::DEBUG, "SPLIT_BLOCK: inserting first half");
				insert_block(first_half,split_order);
				//mm_log.messagef(LogLevel::DEBUG, "SPLIT_BLOCK: dump_state after first insert");
				dump_state();
				//mm_log.messagef(LogLevel::DEBUG, "SPLIT_BLOCK: inserting second half");
				insert_block(second_half,split_order);
				//mm_log.messagef(LogLevel::DEBUG, "SPLIT_BLOCK: dump_state after second insert");
				dump_state();

				return first_half;
		}
	}

	/**
	 * Takes a block in the given source order, and merges it (and it's buddy) into the next order.
	 * This function assumes both the source block and the buddy block are in the free list for the
	 * source order.  If they aren't this function will panic the system.
	 * @param block_pointer A pointer to a pointer containing a block in the pair to merge.
	 * @param source_order The order in which the pair of blocks live.
	 * @return Returns the new slot that points to the merged block.
	 */
	PageDescriptor **merge_block(PageDescriptor **block_pointer, int source_order)
	{
		//mm_log.messagef(LogLevel::DEBUG, "MERGE_BLOCK: merging at order,%d, %lx",source_order, sys.mm().pgalloc().pgd_to_pfn(*block_pointer));
		assert(*block_pointer);

		// Make sure the area_pointer is correctly aligned.
		assert(is_correct_alignment_for_order(*block_pointer, source_order));

		// TODO: Implement this function
		PageDescriptor* buddy = buddy_of(*block_pointer,source_order);
		PageDescriptor* left = *block_pointer;

		remove_block(left,source_order);
		remove_block(buddy,source_order);

		if(buddy>left){
				//mm_log.messagef(LogLevel::DEBUG,"MERGE_BLOCK: going on to insert block with the pgd on lhs, %d",source_order+1);
				return insert_block(left,source_order+1);
				//mm_log.messagef(LogLevel::DEBUG,"MERGE_BLOCK: will never reach here");
		}
		else{
				//mm_log.messagef(LogLevel::DEBUG,"MERGE_BLOCK: going on to insert block with the buddy on rhs, %d",source_order+1);
				return insert_block(buddy,source_order+1);
				//mm_log.messagef(LogLevel::DEBUG,"MERGE_BLOCK: will never reach here");
		}
	}

public:
	/**
	 * Constructs a new instance of the Buddy Page Allocator.
	 */
	BuddyPageAllocator() {
		// Iterate over each free area, and clear it.
		for (unsigned int i = 0; i < ARRAY_SIZE(_free_areas); i++) {
			_free_areas[i] = NULL;
		}
	}

	/**
	 * Allocates 2^order number of contiguous pages
	 * @param order The power of two, of the number of contiguous pages to allocate.
	 * @return Returns a pointer to the first page descriptor for the newly allocated page range, or NULL if
	 * allocation failed.
	 */
	PageDescriptor *alloc_pages(int order) override
	{
		//mm_log.messagef(LogLevel::DEBUG, "ALLOC_PAGES: allocating at %d", order);
		PageDescriptor *slot = _free_areas[order];

		if(order<0 ||  order>=MAX_ORDER){
			//mm_log.messagef(LogLevel::DEBUG, "ALLOC_PAGES: illegal order number %d", order);
			return NULL;
		}

		if (slot != NULL) {
			//mm_log.messagef(LogLevel::DEBUG, "ALLOC_PAGES: the order already has something, removing it now: %lx", sys.mm().pgalloc().pgd_to_pfn(slot));
			remove_block(slot,order);
			return slot;
		}

		else{
			int split_order;
			for(split_order=order+1;split_order<MAX_ORDER;split_order++){
				//mm_log.messagef(LogLevel::DEBUG, "ALLOC_PAGES: looking for something at order: %d",split_order);
				if(_free_areas[split_order])
					break;
			}

			//mm_log.messagef(LogLevel::DEBUG, "ALLOC_PAGES: found at order: %d",split_order);
			while(split_order>order){
				//mm_log.messagef(LogLevel::DEBUG, "ALLOC_PAGES: splitting at order: %d, %lx",split_order, sys.mm().pgalloc().pgd_to_pfn(_free_areas[split_order]));
				slot = split_block(&_free_areas[split_order],split_order);
				split_order--;
			}
			assert(split_order == order);
			////mm_log.messagef(LogLevel::DEBUG, "ALLOC_PAGES: splitting at order: %lx", sys.mm().pgalloc().pgd_to_pfn(_free_areas[split_order]));
			remove_block(slot,order);
			//mm_log.messagef(LogLevel::DEBUG, "ALLOC_PAGES: Dump after final remove");
			dump_state();
			return slot;
		}
	}

	/**
	 * Frees 2^order contiguous pages.
	 * @param pgd A pointer to an array of page descriptors to be freed.
	 * @param order The power of two number of contiguous pages to free.
	 */
	void free_pages(PageDescriptor *pgd, int order) override
	{
		// Make sure that the incoming page descriptor is correctly aligned
		// for the order on which it is being freed, for example, it is
		// illegal to free page 1 in order-1.
		assert(is_correct_alignment_for_order(pgd, order));
		//mm_log.messagef(LogLevel::DEBUG, "FREE_PAGES: freeing %lx", sys.mm().pgalloc().pgd_to_pfn(pgd));
		//mm_log.messagef(LogLevel::DEBUG, "FREE_PAGES: inserting at %d", order);
		PageDescriptor **slot = insert_block(pgd,order);
		//mm_log.messagef(LogLevel::DEBUG, "FREE_PAGES: state after inserting");
		dump_state();
		PageDescriptor *check = _free_areas[order];
		PageDescriptor* buddy = buddy_of(*slot,order);

		while(order<MAX_ORDER-1){

			while (check && check != buddy) {
					//mm_log.messagef(LogLevel::DEBUG,"Order is %d",order);
					//mm_log.messagef(LogLevel::DEBUG, "FREE_PAGE: slot was %lx", sys.mm().pgalloc().pgd_to_pfn(*slot));
					check = check->next_free;
					//mm_log.messagef(LogLevel::DEBUG, "FREE_PAGE: slot is now %lx", sys.mm().pgalloc().pgd_to_pfn(*slot));
			}

			if(check == buddy){
					//mm_log.messagef(LogLevel::DEBUG, "FREE_PAGES: buddy %lx", sys.mm().pgalloc().pgd_to_pfn(buddy_of(*slot,order)));
					//mm_log.messagef(LogLevel::DEBUG, "FREE_PAGES: merging at %d", order);
					slot = merge_block(slot, order);
					order++;
					check = _free_areas[order];
					buddy = buddy_of(*slot,order);
			}
			else{
				break;
			}
		}
	}

	/**
	 * Reserves a specific page, so that it cannot be allocated.
	 * @param pgd The page descriptor of the page to reserve.
	 * @return Returns TRUE if the reservation was successful, FALSE otherwise.
	 */
	bool reserve_page(PageDescriptor *pgd)
	{

		//mm_log.messagef(LogLevel::DEBUG, "RESERVE_PAGE: currently looking for %lx", sys.mm().pgalloc().pgd_to_pfn(pgd));
		int order = 0;

		for(order = 0; order<MAX_ORDER; order++){
				PageDescriptor *slot = _free_areas[order];
				//mm_log.messagef(LogLevel::DEBUG, "RESERVE_PAGE: order: %d", order);
				while (slot) {
						if(order == 0){
							while (slot && pgd != slot) {
								//mm_log.messagef(LogLevel::DEBUG, "RESERVE_PAGE: slot was %lx", sys.mm().pgalloc().pgd_to_pfn(slot));
								slot = slot->next_free;
								//mm_log.messagef(LogLevel::DEBUG, "RESERVE_PAGE: slot is now %lx", sys.mm().pgalloc().pgd_to_pfn(slot));
							}

							if(slot == pgd){
								//mm_log.messagef(LogLevel::DEBUG, "RESERVE_PAGE: order %d, I looked for %lx, I found %lx", order, sys.mm().pgalloc().pgd_to_pfn(pgd),sys.mm().pgalloc().pgd_to_pfn(slot));
								assert(slot == pgd);
								assert(order == 0);
								//mm_log.messagef(LogLevel::DEBUG, "RESERVE_PAGE: found the page at 0th order, will be removing it now");
								remove_block(slot,order);
								dump_state();
								return true;
							}
						}

						if(pgd >= slot && pgd < (slot+pages_per_block(order))){
							//mm_log.messagef(LogLevel::DEBUG, "RESERVE_PAGE: pgd is in this block at order %d, will split now",order);
							//mm_log.messagef(LogLevel::DEBUG, "breaking now");
							slot = split_block(&slot,order);
							order--;

							if(!(pgd >= slot) && !(pgd < slot+pages_per_block(order))){
								//mm_log.messagef(LogLevel::DEBUG, "RESERVE_PAGE: looking for the buddy",order);
								slot = buddy_of(slot,order);
							}
							//mm_log.messagef(LogLevel::DEBUG, "didn't break");
						}

						else{
							//mm_log.messagef(LogLevel::DEBUG, "RESERVE_PAGE: going to the next block");
							if(slot)
								slot = slot->next_free;
							else
								break;
							//mm_log.messagef(LogLevel::DEBUG, "RESERVE_PAGE: at the next block");
							PageDescriptor *test = NULL;
							//mm_log.messagef(LogLevel::DEBUG, "RESERVE_PAGE: slot is now %lx", sys.mm().pgalloc().pgd_to_pfn(test));
						}

				}
			}
			return false;
		}

	/**
	 * Initialises the allocation algorithm.
	 * @return Returns TRUE if the algorithm was successfully initialised, FALSE otherwise.
	 */
	bool init(PageDescriptor *page_descriptors, uint64_t nr_page_descriptors) override
	{
		//mm_log.messagef(LogLevel::DEBUG, "Buddy Allocator Initialising pd=%p, nr=0x%lx", page_descriptors, nr_page_descriptors);

		// TODO: Initialise the free area linked list for the maximum order
		// to initialise the allocation algorithm.

		//TODO: case when it returns false
		auto order = MAX_ORDER-1;
		uint64_t free = nr_page_descriptors;
		auto pgd = page_descriptors;

		//mm_log.messagef(LogLevel::DEBUG, "INIT:");
		// Iterate whilst there is are pages still unallocated and order is not less than 0
		while(free>0){
			//mm_log.messagef(LogLevel::DEBUG, "init: inside while loop");
		  //mm_log.messagef(LogLevel::DEBUG, "order: %d", order);
			//mm_log.messagef(LogLevel::DEBUG, "free pages: %d", free);
			//find the number of blocks that could be allocated to the order given free.
			uint64_t blocks_needed = free/pages_per_block(order);

			//mm_log.messagef(LogLevel::DEBUG, "blocks needed: %d", blocks_needed);
			assert(is_correct_alignment_for_order(pgd, order));

			//if more than 0 blocks can be allocated then
			if(blocks_needed>0){
				//allocate the pointer to the first pgd in the block
				_free_areas[order] = pgd;

				//iterate and move the pointers
				for(uint64_t i=0; i<blocks_needed; i++){
					pgd->next_free = pgd+ pages_per_block(order);
					pgd += pages_per_block(order);
				}

				//pgd += pages_per_block(order);
			}

			//calculate the new number of free pages
			free = free % (pages_per_block(order));
			//mm_log.messagef(LogLevel::DEBUG, "updated free: %d", free);
			//go to the next order
			order--;
		}

		//mm_log.messagef(LogLevel::DEBUG, "returning true");

		// while(true) {
		// 	dump_state();
		// 	reserve_page(page_descriptors);
		// 	dump_state();
		// }
		return true;
	}

	/**
	 * Returns the friendly name of the allocation algorithm, for debugging and selection purposes.
	 */
	const char* name() const override { return "buddy"; }

	/**
	 * Dumps out the current state of the buddy system
	 */
	void dump_state() const override
	{
		// Print out a header, so we can find the output in the logs.
		//mm_log.messagef(LogLevel::DEBUG, "BUDDY STATE:");

		// Iterate over each free area.
		for (unsigned int i = 0; i < ARRAY_SIZE(_free_areas); i++) {
			char buffer[256];
			snprintf(buffer, sizeof(buffer), "[%d] ", i);

			// Iterate over each block in the free area.
			PageDescriptor *pg = _free_areas[i];
			while (pg) {
				// Append the PFN of the free block to the output buffer.
				snprintf(buffer, sizeof(buffer), "%s%lx ", buffer, sys.mm().pgalloc().pgd_to_pfn(pg));
				pg = pg->next_free;
			}

			//mm_log.messagef(LogLevel::DEBUG, "%s", buffer);
		}
	}


private:
	PageDescriptor *_free_areas[MAX_ORDER];
};

/* --- DO NOT CHANGE ANYTHING BELOW THIS LINE --- */

/*
 * Allocation algorithm registration framework
 */
RegisterPageAllocator(BuddyPageAllocator);
