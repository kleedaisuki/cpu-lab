#include "domain/matrix_dot/matrix_dot_cuda.hpp"

#include <cuda_runtime.h>

#include <cstddef>
#include <stdexcept>
#include <string>
#include <utility>

namespace cpu_lab::domain::matrix_dot
{

    namespace
    {
        /**
         * @brief 校验输入形状（validate shape）；Validate matrix/vector compatibility.
         * @param matrix 输入矩阵（input matrix）。
         * @param vector 输入向量（input vector）。
         * @return 无返回值；No return value.
         */
        void validate_shape(const Matrix &matrix, const Vector &vector)
        {
            if (matrix.rows() != vector.size())
            {
                throw std::invalid_argument("matrix_dot_cuda requires matrix.rows() == vector.size().");
            }
        }

        /**
         * @brief 校验线程块大小（validate block size）；Validate threads-per-block argument.
         * @param threads_per_block 每个线程块线程数（threads per block）。
         * @return 无返回值；No return value.
         */
        void validate_block_size(const std::size_t threads_per_block)
        {
            if (threads_per_block == 0U)
            {
                throw std::invalid_argument("matrix_dot_cuda requires threads_per_block > 0.");
            }
        }

        /**
         * @brief CUDA 错误检查（CUDA error check）；Throw std::runtime_error on CUDA failure.
         * @param code CUDA 返回码（CUDA status code）。
         * @param context 错误上下文（error context）。
         * @return 无返回值；No return value.
         */
        void check_cuda(const cudaError_t code, const char *const context)
        {
            if (code != cudaSuccess)
            {
                throw std::runtime_error(
                    std::string("matrix_dot_cuda failed at ") + context + ": " + cudaGetErrorString(code));
            }
        }

        /**
         * @brief 设备内存托管器（device memory guard）；RAII wrapper for cudaMalloc/cudaFree.
         */
        class DeviceBuffer final
        {
        public:
            /**
             * @brief 构造函数（constructor）；Allocate device memory for count doubles.
             * @param count 元素数量（element count）。
             */
            explicit DeviceBuffer(const std::size_t count)
            {
                if (count == 0U)
                {
                    return;
                }
                check_cuda(cudaMalloc(reinterpret_cast<void **>(&ptr_), count * sizeof(double)), "cudaMalloc");
            }

            /**
             * @brief 析构函数（destructor）；Release owned device memory.
             */
            ~DeviceBuffer()
            {
                if (ptr_ != nullptr)
                {
                    (void)cudaFree(ptr_);
                }
            }

            DeviceBuffer(const DeviceBuffer &) = delete;
            DeviceBuffer &operator=(const DeviceBuffer &) = delete;

            /**
             * @brief 获取原始指针（get pointer）；Access managed device pointer.
             * @return 设备指针（device pointer）。
             */
            [[nodiscard]] double *get() const noexcept
            {
                return ptr_;
            }

        private:
            /** @brief 设备指针（device pointer）；Owned CUDA device buffer. */
            double *ptr_{nullptr};
        };

        /**
         * @brief CUDA 列点积内核（CUDA column-dot kernel）；Each thread computes one output column.
         * @param matrix 行主序矩阵数据（row-major matrix data）。
         * @param vector 输入向量数据（input vector data）。
         * @param rows 矩阵行数（matrix rows）。
         * @param cols 矩阵列数（matrix cols）。
         * @param output 输出向量数据（output vector data）。
         */
        __global__ void matrix_dot_kernel(
            const double *matrix,
            const double *vector,
            const std::size_t rows,
            const std::size_t cols,
            double *output)
        {
            const std::size_t col = (static_cast<std::size_t>(blockIdx.x) * blockDim.x) + threadIdx.x;
            if (col >= cols)
            {
                return;
            }

            double sum = 0.0;
            for (std::size_t row = 0U; row < rows; ++row)
            {
                sum += matrix[(row * cols) + col] * vector[row];
            }
            output[col] = sum;
        }
    } // namespace

    Vector matrix_dot_cuda(
        const Matrix &matrix,
        const Vector &vector,
        const std::size_t threads_per_block)
    {
        validate_shape(matrix, vector);
        validate_block_size(threads_per_block);

        Vector output(matrix.cols(), 0.0);
        if (matrix.cols() == 0U)
        {
            return output;
        }

        int device = 0;
        check_cuda(cudaGetDevice(&device), "cudaGetDevice");

        cudaDeviceProp props{};
        check_cuda(cudaGetDeviceProperties(&props, device), "cudaGetDeviceProperties");
        if (threads_per_block > static_cast<std::size_t>(props.maxThreadsPerBlock))
        {
            throw std::invalid_argument("matrix_dot_cuda threads_per_block exceeds device maxThreadsPerBlock.");
        }

        DeviceBuffer d_matrix(matrix.size());
        DeviceBuffer d_vector(vector.size());
        DeviceBuffer d_output(output.size());

        check_cuda(
            cudaMemcpy(
                d_matrix.get(),
                matrix.data().data(),
                matrix.size() * sizeof(double),
                cudaMemcpyHostToDevice),
            "cudaMemcpy(matrix H2D)");

        check_cuda(
            cudaMemcpy(
                d_vector.get(),
                vector.data().data(),
                vector.size() * sizeof(double),
                cudaMemcpyHostToDevice),
            "cudaMemcpy(vector H2D)");

        const std::size_t blocks = (matrix.cols() + threads_per_block - 1U) / threads_per_block;
        matrix_dot_kernel<<<static_cast<unsigned int>(blocks), static_cast<unsigned int>(threads_per_block)>>>(
            d_matrix.get(),
            d_vector.get(),
            matrix.rows(),
            matrix.cols(),
            d_output.get());

        check_cuda(cudaGetLastError(), "kernel launch");
        check_cuda(cudaDeviceSynchronize(), "cudaDeviceSynchronize");

        check_cuda(
            cudaMemcpy(
                output.data().data(),
                d_output.get(),
                output.size() * sizeof(double),
                cudaMemcpyDeviceToHost),
            "cudaMemcpy(output D2H)");

        return output;
    }

    MatrixDotCuda::MatrixDotCuda(Matrix matrix, Vector vector, const std::size_t threads_per_block)
        : matrix_(std::move(matrix)),
          vector_(std::move(vector)),
          threads_per_block_(threads_per_block),
          output_(matrix_.cols(), 0.0)
    {
        validate_shape(matrix_, vector_);
        validate_block_size(threads_per_block_);
    }

    std::string_view MatrixDotCuda::algorithm_name() const noexcept
    {
        return "matrix_dot_cuda";
    }

    void MatrixDotCuda::run_once()
    {
        output_ = matrix_dot_cuda(matrix_, vector_, threads_per_block_);
    }

    const Vector &MatrixDotCuda::output() const noexcept
    {
        return output_;
    }

    std::uint64_t MatrixDotCuda::output_fingerprint() const noexcept
    {
        return output_.fingerprint();
    }

} // namespace cpu_lab::domain::matrix_dot