#ifndef BLOCKPARTITION_H
#define BLOCKPARTITION_H

const int BLOCK_SIZE_BITS = 8;

const int BLOCK_SIZE = 1 << BLOCK_SIZE_BITS;
const int BLOCK_MASK = ~(BLOCK_SIZE - 1);

template<typename T>
class BlockPartition {
    public:
        BlockPartition(int width, int height, T no_data)
            : width(width), height(height), no_data_value(no_data)
        {
            width_blocks = ((width + BLOCK_SIZE - 1) & BLOCK_MASK) >> BLOCK_SIZE_BITS;

            int blocks_needed = width_blocks * (((height + BLOCK_SIZE - 1) & BLOCK_MASK) >> BLOCK_SIZE_BITS);

            blocks = new T*[blocks_needed];

            for(int i = 0; i < blocks_needed; i++) {
                blocks[i] = nullptr;
            }
        }

        BlockPartition(const BlockPartition&) = delete;
        BlockPartition(BlockPartition&&) = delete;

        BlockPartition& operator=(const BlockPartition&) & = delete;
        BlockPartition& operator=(BlockPartition&&) & = delete;

        ~BlockPartition() {
            int blocks_needed = width_blocks * (((height + BLOCK_SIZE - 1) & BLOCK_MASK) >> BLOCK_SIZE_BITS);

            for (int i = 0; i < blocks_needed; i++) {
                if (blocks[i] != nullptr) {
                    delete[] blocks[i];
                }
            }
            delete[] blocks;
        }

        T& get_pixel(int gx, int gy) {
            // Find block:
            int bx = (gx & BLOCK_MASK) >> BLOCK_SIZE_BITS;
            int by = (gy & BLOCK_MASK) >> BLOCK_SIZE_BITS;

            int id = bx + by * width_blocks;

            T* block_data = blocks[id];

            if (block_data == nullptr) {
                // Create new block
                block_data = new T[BLOCK_SIZE * BLOCK_SIZE];

                for(int i = 0; i < BLOCK_SIZE*BLOCK_SIZE; i++) {
                    block_data[i] = no_data_value;
                }

                blocks[id] = block_data;
            }

            int x = gx & ~BLOCK_MASK;
            int y = gy & ~BLOCK_MASK;

            //printf("getting %d %d -> %d %d -> %d %d\n", gx, gy, bx, by, x, y);

            return block_data[x + y*BLOCK_SIZE];
        }

        T getData(int gx, int gy) {
            return get_pixel(gx, gy);
        }

        void setData(int gx, int gy, T val) {
            get_pixel(gx, gy) = val;
        }

        void addToData(int gx, int gy, T val) {
            get_pixel(gx, gy) += val;
        }

        void share() {}

    private:
        int width, height, width_blocks;
        T no_data_value;

        T** blocks;
};

#endif //BLOCKPARTITION_H
