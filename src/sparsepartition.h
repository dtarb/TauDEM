#ifndef SPARSEPARTITION_H
#define SPARSEPARTITION_H

// Sparse partition that internally stores the raster as tiles. 
// 
// It greatly reduces the memory footprint for temporary
// rasters that are used sparsely during computation.

#include <cstring>

#include <vector>
#include <memory>

#include <mpi.h>
#include "const.h"

const int BLOCK_SIZE_BITS = 8;

const int BLOCK_SIZE = 1 << BLOCK_SIZE_BITS;
const int BLOCK_MASK = ~(BLOCK_SIZE - 1);

class AsyncPartition { 
    public:
        virtual void* get_async_buf() = 0;
        virtual size_t get_async_buf_size() = 0;

        virtual void async_load_buf(bool top, std::vector<node>& changes) = 0;
        virtual void async_store_buf(bool top) = 0;
};

template<typename T>
class SparsePartition : public AsyncPartition {
    public:
        SparsePartition(T no_data)
        {
            width = 0;
            width_blocks = 0;
            height = 0;
            blocks = nullptr;

            no_data_value = no_data;
        }

        SparsePartition(int width, int height, T no_data)
            : width(width), height(height), no_data_value(no_data)
        {
            width_blocks = ((width + BLOCK_SIZE - 1) & BLOCK_MASK) >> BLOCK_SIZE_BITS;

            int blocks_needed = width_blocks * (((height + 2 + BLOCK_SIZE - 1) & BLOCK_MASK) >> BLOCK_SIZE_BITS);

            blocks = new T*[blocks_needed];

            for(int i = 0; i < blocks_needed; i++) {
                blocks[i] = nullptr;
            }

            border_buf = std::unique_ptr<T[]>(new T[width_blocks * BLOCK_SIZE]);
            async_buf = std::unique_ptr<T[]>(new T[width_blocks * BLOCK_SIZE]);
        }

        SparsePartition(const SparsePartition& bp) = delete;
        SparsePartition(SparsePartition&& bp) = default;

        SparsePartition& operator=(const SparsePartition&) = delete;
        SparsePartition& operator=(SparsePartition&& bp) {
            width = bp.width;
            height = bp.height;
            width_blocks = bp.width_blocks;
            no_data_value = bp.no_data_value;

            free_blocks();
            blocks = bp.blocks;
            bp.blocks = nullptr;

            border_buf = std::move(bp.border_buf);
            async_buf = std::move(bp.async_buf);

			return *this;
        };

        ~SparsePartition() {
            free_blocks();
        }

        T& get_pixel(int gx, int gy) const {
            gy += 1;

            T* block_data = get_block(gx, gy, true);

            int x = gx & ~BLOCK_MASK;
            int y = gy & ~BLOCK_MASK;

            return block_data[x + y*BLOCK_SIZE];
        }

        T getData(int gx, int gy) const {
            if (get_block(gx, gy + 1) == nullptr)
                return no_data_value;

            return get_pixel(gx, gy);
        }

        void setData(int gx, int gy, T val) {
            get_pixel(gx, gy) = val;
        }

        void addToData(int gx, int gy, T val) {
            get_pixel(gx, gy) += val;
        }

        void share(bool top=true, bool bottom=true) {
            int rank, size;
            MPI_Comm_rank(MPI_COMM_WORLD, &rank);
            MPI_Comm_size(MPI_COMM_WORLD, &size);

            if (size == 1) return;

            T* buf = new T[width_blocks * BLOCK_SIZE];
            const int buf_size = sizeof(T) * width_blocks * BLOCK_SIZE;

            if (top && rank > 0) {
                load_border_buf(buf, 1);

                MPI_Sendrecv_replace(buf, buf_size, MPI_BYTE, rank-1, 0,
                    rank-1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

                //printf("R%d: SENDRECV 1/0\n", rank);

                store_border_buf(buf, 0);
            }

            if (bottom && rank < size - 1) {
                // load buf
                load_border_buf(buf, height);
               
                MPI_Sendrecv_replace(buf, buf_size, MPI_BYTE, rank+1, 0,
                    rank+1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

                //printf("R%d: SENDRECV height/height+1\n", rank);

                // store buf
                store_border_buf(buf, height + 1);
            }

            delete[] buf;
        }

        // Async stuff
        void* get_async_buf() {
            return async_buf.get();
        }

        size_t get_async_buf_size() {
            return width_blocks * BLOCK_SIZE * sizeof(T);
        }

        void async_load_buf(bool top, std::vector<node>& changes) {
            auto y = top ? 0 : height + 1;

            load_border_buf(border_buf.get(), y);

            for (int x = 0; x < width; x++) {
                if (border_buf[x] != async_buf[x])
                    changes.push_back(node(x, y - 1));
            }

            store_border_buf(async_buf.get(), y); 
        }

        void async_store_buf(bool top) {
            auto y = top ? 1 : height;

            load_border_buf(async_buf.get(), y);
        }

    private:
        void load_border_buf(T* buf, int y) {
            for (int x = 0; x < width; x += BLOCK_SIZE) {
                T* block = get_stride(x, y, false);

                if (block == nullptr) {
                    for(int i = 0; i < BLOCK_SIZE; i++) {
                        buf[x + i] = no_data_value;
                    }
                } else {
                    memcpy(&buf[x], block, sizeof(T)*BLOCK_SIZE);
                }
            }          
        }

        void store_border_buf(T* buf, int y) {
            for (int x = 0; x < width; x += BLOCK_SIZE) {
                T* block = get_stride(x, y, false);

                if (block == nullptr) {
                    // Check if all values are no data to save memory
                    for(int i = 0; i < BLOCK_SIZE; i++) {
                        if (buf[x + i] != no_data_value)
                        {
                            block = get_stride(x, y, true);
                            break;
                        }
                    }
                }

                if (block != nullptr) {
                    memcpy(block, &buf[x], sizeof(T)*BLOCK_SIZE);
                }
            }
        }

        T* get_block(int gx, int gy, bool create=false) const {
            int bx = (gx & BLOCK_MASK) >> BLOCK_SIZE_BITS;
            int by = (gy & BLOCK_MASK) >> BLOCK_SIZE_BITS;

            int id = bx + by * width_blocks;

            if (blocks == nullptr)
                return nullptr;

            T* block_data = blocks[id];

            if (block_data == nullptr && create) {
                // Create new block
                block_data = new T[BLOCK_SIZE * BLOCK_SIZE];

                std::fill(block_data, block_data + BLOCK_SIZE*BLOCK_SIZE, no_data_value);

                blocks[id] = block_data;
            }

            return block_data;
        }

        T* get_stride(int gx, int gy, bool create=false) {
            T* block = get_block(gx, gy, create);

            if (block == nullptr)
                return nullptr;

            int y = gy & ~BLOCK_MASK;

            return &block[y*BLOCK_SIZE];
        }

        void free_blocks() {
            if (blocks != nullptr) { 
                int blocks_needed = width_blocks * (((height + 2 + BLOCK_SIZE - 1) & BLOCK_MASK) >> BLOCK_SIZE_BITS);

                for (int i = 0; i < blocks_needed; i++) {
                    if (blocks[i] != nullptr) {
                        delete[] blocks[i];
                    }
                }
                delete[] blocks;
            }
        }

        int width, height, width_blocks;
        T no_data_value;

        T** blocks;

        std::unique_ptr<T[]> border_buf;
        std::unique_ptr<T[]> async_buf;
};

#endif //SPARSEPARTITION_H
