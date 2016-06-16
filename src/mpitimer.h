#ifndef MPI_TIMER_H
#define MPI_TIMER_H

#include <cstdio>
#include <string>

#include <numeric>
#include <vector>
#include <unordered_map>

#include <mpi.h>

class MPITimer
{
    public:
        MPITimer() : level(0), cur_id(0)
        {
            offset = MPI_Wtime();
        }

        void start(std::string name) {
            timings[name].name = name;
            timings[name].id = cur_id++;
            timings[name].level = level++;
            timings[name].start = MPI_Wtime() - offset;
        }

        double end(std::string name) {
            timings[name].end = MPI_Wtime() - offset;
            level--;

            return timings[name].end;
        }

        void stop() {
            int rank, size;

            MPI_Comm_rank(MPI_COMM_WORLD, &rank);
            MPI_Comm_size(MPI_COMM_WORLD, &size);

            std::vector<timing> ordered;
            ordered.reserve(timings.size());

            for (auto& x: timings) {
                ordered.push_back(x.second);
            }
           
            std::sort(ordered.begin(), ordered.end(), [](const timing& a, const timing& b) { return a.id < b.id; });

            // FIXME

            // FIXME: some ranks might no enter all timings
            double *send = new double[ordered.size() * 2];

            double *p = send;
            for (auto& t : ordered) {
                *p++ = t.start;
                *p++ = t.end;
            }

            double *results = nullptr;
            if (rank == 0) {
               results = new double[ordered.size() * 2 * size];
            }

            MPI_Gather(send, ordered.size() * 2, MPI_DOUBLE,
                    results, ordered.size() * 2, MPI_DOUBLE,
                    0, MPI_COMM_WORLD); 

            delete[] send;
            
            if (rank == 0) {
                agg_timings.reserve(ordered.size());

                for (size_t n = 0; n < ordered.size(); n++) {
                    aggregated_timing at;

                    at.name = ordered[n].name;
                    at.level = ordered[n].level;

                    at.start.reserve(size);
                    at.end.reserve(size);

                    for (int i = 0; i < size; i++) {
                        at.start.push_back(results[i*2*ordered.size() + n*2]);
                        at.end.push_back(results[i*2*ordered.size() + n*2 + 1]);
                    }

                    agg_timings.push_back(at);
                }

                delete[] results;

                printf("Processors: %d\n", size);

                for (auto& x : agg_timings) {
                    std::vector<double> times;
                    times.reserve(x.start.size());

                    for (size_t i = 0; i < x.start.size(); i++) {
                        times.push_back(x.end[i] - x.start[i]);
                    }

                    double avg = std::accumulate(times.begin(), times.end(), 0.0) / times.size();

                    double min = *std::min_element(times.begin(), times.end());
                    double max = *std::max_element(times.begin(), times.end());

                    for (int i = 0; i < x.level; i++) {
                        printf(" ");
                    }

                    printf("%s: %.2f (%.2f - %.2f)\n", x.name.c_str(), avg, min, max);

                    //for (auto t : times) {
                        //printf("%.2f ", t);
                    //}

                    //printf("]\n");
                }

                
            }
        }

        void save(const char *fn) {
            int rank;
            MPI_Comm_rank(MPI_COMM_WORLD, &rank);

            if (rank != 0) return;

            FILE* fp = fopen(fn, "a");

            if (!fp) {
                std::perror("opening failed");
            }

            int size;
            MPI_Comm_size(MPI_COMM_WORLD, &size);
            
            fprintf(fp, "np,%d\n", size);
            fprintf(fp, "offset,%f\n", offset);

            for (auto& x : agg_timings) {
                std::vector<double> times;;

                times.reserve(x.start.size());

                for (size_t i = 0; i < x.start.size(); i++) {
                    times.push_back(x.end[i] - x.start[i]);
                }

                fprintf(fp, "%s,start", x.name.c_str());
                for (auto t : x.start) { fprintf(fp, ",%f", t); }
                fprintf(fp, "\n");
 
                fprintf(fp, "%s,end", x.name.c_str());
                for (auto t : x.end) { fprintf(fp, ",%f", t); }
                fprintf(fp, "\n");
                
                fprintf(fp, "%s,duration", x.name.c_str());
                for (auto t : times) { fprintf(fp, ",%f", t); }
                fprintf(fp, "\n");
            }

            fprintf(fp, "=============\n");

            fclose(fp);
        }

    private:
        double offset;

        int level;
        int cur_id;

        struct timing {
            int id; // used for ordering

            std::string name;
            int level;
            double start;
            double end;
        };

        struct aggregated_timing {
            std::string name;
            int level;
            std::vector<double> start;
            std::vector<double> end;

            aggregated_timing() : start(), end() {}
        };

        std::unordered_map<std::string, timing> timings;
        std::vector<aggregated_timing> agg_timings;
};

#endif//MPI_TIMER_H
