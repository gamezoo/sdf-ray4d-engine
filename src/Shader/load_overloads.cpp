/*****************************************************
 * Partial Class: Shader
 * Members: Load Function Overloads (Public/Private)
 *****************************************************/

#include <QtConcurrentRun>

#include "Shader.hpp"
#include "SPIRVCompiler.hpp"

using namespace sdfRay4d;

/**
 * PUBLIC
 *
 * @brief
 * Helper function to load a shader module asynchronously or
 * wait for the shader compilation to be finished, based on
 * type of the shader file
 *
 * if shader file is not spv, it can optionally serialize multiple
 * GLSL shader partial files (separate static shader instructions) into
 * a single bytecode array, compile it into SPIRV shader and finally
 * load it as a shader module
 * @param[in] _filePath (QString) main shader file path
 * @param[in] _partialFilePaths (QStringList) shader's partial file paths (optional)
 *
 * TODO:
 * Make this function a variadic template with parameter pack
 * for filePaths. The complexity would be to make a separate
 * header file to have decl/definition in one file and also
 * the strings need to be either views or references so not to
 * copy over, initializer_list collection might be worth looking at.
 */
void Shader::load(
  const QString &_filePath,
  const QStringList &_partialFilePaths
)
{
  reset();

  m_isLoading = true;

  auto fileExtension = _filePath.toStdString();
  fileExtension = fileExtension.substr(fileExtension.find_last_of('.') + 1);

  /**
   * @note
   *
   * As compiling the shader cannot be done asynchronously,
   * there could be two ways of implementing this:
   *
   * - separate the compilation functionality to be outside the async callback function
   *   and only load shader module async without the need to explicitly make it wait to be completed
   *
   * - put both compilation and loading the shader module inside the async callback function
   *   and make the thread to explicitly wait for the job to be completed.
   *
   * In both methods, the shader compilation will block the process to be finished and there will be
   * no extra runtime performance penalty using one over another.
   *
   * Explicit wait function may only slightly increase the length of the code path and
   * therefore increase the build time very slightly (which is already optimized by cmake's UNITY_BUILD/PCH).
   *
   * So, for the sake of code readability/maintainability, it may be cleaner to put everything inside the async
   * callback function.
   */
  m_worker = QtConcurrent::run([fileExtension, _filePath, _partialFilePaths, this]()
  {
    auto isPrecompiled = fileExtension == "spv";
    auto rawBytes = getFileBytes(m_shadersPath + _filePath);

    std::vector<uint32_t> spvBytes;
    std::string log;

    if(isPrecompiled && !_partialFilePaths.isEmpty())
    {
      qWarning("\nWARNING: Partial files cannot be merged back into precompiled SPIRV files. Therefore, they're ignored\n");
    }

    if(!isPrecompiled)
    {
      // only if there is any partial shader helper file
      if(!_partialFilePaths.isEmpty())
      {
        serialize(_partialFilePaths, rawBytes);
      }

      /**
       * TODO: make SPIRVCompiler::compile thread-safe with lock/unlock?
       *
       * @note
       *
       * SPV Compiler is not thread-safe and cannot synchronously compile
       * multiple shaders. It should only be called once per process, not per thread.
       */
      if(
        !SPIRVCompiler::compile(
          getShaderStage(fileExtension),
          rawBytes, "main", spvBytes, log
        )
      )
      {
        qWarning("Failed to compile shader: %s", log.c_str());
        return Data();
      }
    }

    return load(
      spvBytes, // runtime compiled spirv bytes (if available, otherwise empty)
      rawBytes, // pre-compiled spv file bytes
      isPrecompiled
    );
  });

  /**
   * This will avoid triggering synchronous spirv compilation
   * as it will crash the app otherwise. This is due to the
   * glslang compiler initialize call being only per process
   * and not per thread.
   */
  if(fileExtension != "spv")
  {
    // equivalent to std::future::wait_for with std::future_status::ready
    m_worker.waitForFinished();
  }
}

/**
 * PUBLIC
 *
 * @brief
 * @param[in] _partialFilePaths
 */
void Shader::preload(
//  const QString &_replaceItem,
  const QStringList &_partialFilePaths
)
{
  for (auto i = 0; i < _partialFilePaths.size(); i++)
  {
    auto filePath = _partialFilePaths[i];
//
//    if(filePath.isEmpty())
//    {
//      m_rawBytes.append(QByteArray("PLACEHOLDER\n"));
//
//      continue;
//    }

    m_rawBytes.append(getFileBytes(m_shadersPath + filePath));
  }

//  for(auto &filePath : _partialFilePaths)
//  {
//    if(filePath.isEmpty())
//    {
//
//    }
//  }
}

/**
 * PUBLIC
 *
 * @brief
 * @param[in] _shaderData
 */
void Shader::load(
  const std::string &_shaderData
)
{
  reset();

  m_isLoading = true;

  m_worker = QtConcurrent::run([_shaderData, this]()
  {
    serialize(QByteArray(_shaderData.data()));

    std::vector<uint32_t> spvBytes;
    std::string log;

    /**
     * TODO: make SPIRVCompiler::compile thread-safe with lock/unlock?
     *
     * @note
     *
     * SPV Compiler is not thread-safe and cannot synchronously compile
     * multiple shaders. It should only be called once per process, not per thread.
     */
    if(
      !SPIRVCompiler::compile(
        getShaderStage("frag"),
        m_rawBytes, "main", spvBytes, log
        )
      )
    {
      qWarning("Failed to compile shader: %s", log.c_str());
      return Data();
    }

    return load(
      spvBytes // runtime compiled spirv bytes (if available, otherwise empty)
    );
  });

  /**
   * This will avoid triggering synchronous spirv compilation
   * as it will crash the app otherwise. This is due to the
   * glslang compiler initialize call being only per process
   * and not per thread.
   */
  m_worker.waitForFinished();

//  std::cout << m_rawBytes.constData();
}

/**
 * PRIVATE
 *
 * @brief Helper function to create/load shader module from the passed spirv bytecodes
 * @param[in] _spvBytes {std::vector<uint32_t>} compiled spirv bytecodes at runtime
 * @param[in] _rawSPVBytes {const QByteArray} pre-compiled spirv bytecodes from spv files
 * @param[in] _isPrecompiled {bool}
 * @return data {Shader::Data} struct
 */
Shader::Data Shader::load(
  std::vector<uint32_t> &_spvBytes,
  const QByteArray &_rawSPVBytes,
  bool _isPrecompiled
)
{
  Data data;

  shader::Info shaderInfo = {};
  shaderInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  shaderInfo.codeSize = !_isPrecompiled
                        ? _spvBytes.size() * sizeof(uint32_t)
                        : _rawSPVBytes.size();
  shaderInfo.pCode = !_isPrecompiled
                     ? _spvBytes.data()
                     : reinterpret_cast<const uint32_t *>(_rawSPVBytes.constData());

  auto result = m_deviceFuncs->vkCreateShaderModule(
    m_device,
    &shaderInfo,
    nullptr,
    &data.shaderModule
  );

  if (result != VK_SUCCESS)
  {
    qWarning("Failed to create shader module: %d", result);

    return data;
  }

  return data;
}
