

package cmake;

import com.google.common.collect.Sets;
import generators.GenericGenerator;
import generators.PlatformType;
import generators.ProjectSourcesSetup;
import generators.SolutionType;
import library.Config;
import library.Library;
import library.SystemLibrary;
import project.*;
import project.dependencies.Dependency;
import project.dependencies.DependencyType;
import utils.GeneratedFile;
import utils.GeneratedFilesCollection;
import utils.KeyValueTable;

import java.io.IOException;
import java.nio.file.*;
import java.nio.file.attribute.FileTime;
import java.util.*;
import java.util.stream.Collector;
import java.util.stream.Collectors;

public class CMakeGenerator extends GenericGenerator {

    public CMakeGenerator() {
    }

    @Override
    public String toString() {
    return "CMake (CLion compatible)";
  }

    @Override
    public void generateContent(Solution sol, KeyValueTable params, GeneratedFilesCollection files) {
        // Parse parameters and generate solution and project setup
        try {
            CMakeSolutionSetup solutionSetup = new CMakeSolutionSetup(sol, params);

            // Generate the project content
            // NOTE: we only generate content for enabled projects
            solutionSetup.cmakeProjects.stream().forEach(p -> generateProject(p, files));

            // Generate the RTTI glue code
            generateRTIIForSolution(solutionSetup, files);

            // Generate the solution
            generateSolution(solutionSetup, files);

        } catch (Throwable e) {
            throw new RuntimeException("Project generation failed", e);
        }
    }

    private void generateProject(CMakeSourcesProjectSetup project, GeneratedFilesCollection files) {

        // generate the common files first
        generateGenericProjectFiles(project, files);

        // generate the project CMakeLists.txt
        generateProjectMainFile(project, files);
    }

    private List<File> getProjectFiles(CMakeSourcesProjectSetup project) {

        List<File> ret = new ArrayList<>();

        // get the normal files
        project.originalProject.files.stream().filter(pf -> pf.platforms.contains(project.solutionSetup.originalSolution.platformType)).filter(pf -> pf.shortName.equals("build.cpp")).forEach(pf -> ret.add(pf));
        project.originalProject.files.stream().filter(pf -> pf.platforms.contains(project.solutionSetup.originalSolution.platformType)).filter(pf -> !pf.shortName.equals("build.cpp")).forEach(pf -> ret.add(pf));

        // add "automatic" includes and sources
        project.localAdditionalHeaderFiles.forEach(pf -> ret.add(createFileWrapperForAutogeneratedFile(pf, FileType.HEADER)));
        project.localAdditionalSourceFiles.forEach(pf -> ret.add(createFileWrapperForAutogeneratedFile(pf, FileType.SOURCE)));
        project.localAdditionalVSIXManifestFiles.forEach(pf -> ret.add(createFileWrapperForAutogeneratedFile(pf, FileType.VSIXMANIFEST)));

        return ret;
    }

    private File createFileWrapperForAutogeneratedFile(Path physicalPath, FileType fileType) {
        File f = File.CreateVirtualFile(physicalPath, physicalPath.getFileName(), fileType);
        f.attributes.addValue("pch", "disable");
        f.attributes.addValue("generated", "true");
        f.attributes.addValue("filter", "_auto");
        return f;
    }

    private void generateProjectMainFile(CMakeSourcesProjectSetup project, GeneratedFilesCollection files) {
        String name = project.originalProject.mergedName;

        GeneratedFile f = files.createFile(project.localProjectFilePath);

        f.writeln("# Boomer Engine v4");
        f.writeln("# Written by Tomasz \"Rex Dex\" Jonarski");
        f.writeln("# AutoGenerated file. Please DO NOT MODIFY.");
        f.writeln("");
        f.writelnf("project(%s)", name);
        f.writeln("");

        f.writeln("set(CMAKE_CXX_STANDARD 17)");
        f.writeln("set(CMAKE_CXX_STANDARD_REQUIRED ON)");
        f.writeln("set(CMAKE_CXX_EXTENSIONS OFF)");

        f.writelnf("add_definitions(-DPROJECT_NAME=%s)", name);
        f.writeln("string(TOUPPER \"${CMAKE_BUILD_TYPE}\" uppercase_CMAKE_BUILD_TYPE)");
        f.writeln("set(CMAKE_CONFIGURATION_TYPES \"Debug;Checked;Release\")");

        if (project.solutionSetup.originalSolution.solutionType.buildAsLibs) {
            f.writelnf("add_definitions(-DBUILD_AS_LIBS)", name);
        } else {
            String exportsMacroName = project.originalProject.mergedName.toUpperCase() + "_EXPORTS";
            f.writelnf("add_definitions(-D%s)", exportsMacroName);

            if (!project.originalProject.attributes.hasKey("app"))
                f.writelnf("add_definitions(-DBUILD_DLL)", name);
        }

        f.writelnf("set(CMAKE_EXE_LINKER_FLAGS_CHECKED \"${CMAKE_EXE_LINKER_FLAGS_RELEASE}\")");
        f.writelnf("set(CMAKE_SHARED_LINKER_FLAGS_CHECKED \"${CMAKE_SHARED_LINKER_FLAGS_CHECKED}\")");

        f.writeln("set( CMAKE_CXX_FLAGS_DEBUG  \"${CMAKE_CXX_FLAGS_DEBUG} ${CMAKE_CXX_FLAGS} -DBUILD_DEBUG -D_DEBUG -DDEBUG\")");
        f.writeln("set( CMAKE_CXX_FLAGS_CHECKED  \"${CMAKE_CXX_FLAGS_CHECKED} ${CMAKE_CXX_FLAGS} -DBUILD_CHECKED -DNDEBUG\")");
        f.writeln("set( CMAKE_CXX_FLAGS_RELEASE  \"${CMAKE_CXX_FLAGS_RELEASE} ${CMAKE_CXX_FLAGS} -DBUILD_RELEASE -DNDEBUG\")");

        if (project.solutionSetup.originalSolution.platformType == PlatformType.WINDOWS || project.solutionSetup.originalSolution.platformType == PlatformType.UWP) {
            f.writelnf("add_definitions(-DUNICODE -D_UNICODE -D_WIN64 -D_WINDOWS -DWIN32_LEAN_AND_MEAN -DNOMINMAX)");
            f.writelnf("add_definitions(-D_SILENCE_ALL_CXX17_DEPRECATION_WARNINGS)");
            f.writelnf("add_definitions(-D_SILENCE_TR1_NAMESPACE_DEPRECATION_WARNING)");
            f.writelnf("add_definitions(-D_SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING)");
            f.writelnf("add_definitions(-D_SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING)");

            if (project.originalProject.attributes.hasKey("console")) {
                f.writelnf("add_definitions(-DCONSOLE)");
            }
        } else {
            f.writeln("set(CMAKE_CXX_FLAGS \"${CMAKE_CXX_FLAGS} -pthread -fno-exceptions\")");

            if (!project.originalProject.attributes.hasKey("nosymbols"))
                f.writeln("set(CMAKE_CXX_FLAGS \"${CMAKE_CXX_FLAGS} -g\")");

            f.writeln("set( CMAKE_CXX_FLAGS_DEBUG  \"${CMAKE_CXX_FLAGS_DEBUG} ${CMAKE_CXX_FLAGS} -O0 -fstack-protector-all \")");
            f.writeln("set( CMAKE_CXX_FLAGS_CHECKED  \"${CMAKE_CXX_FLAGS_CHECKED} ${CMAKE_CXX_FLAGS} -O2 -fstack-protector-all \")");
            f.writeln("set( CMAKE_CXX_FLAGS_RELEASE  \"${CMAKE_CXX_FLAGS_RELEASE} ${CMAKE_CXX_FLAGS} -O3 -fno-stack-protector\")");
        }

        if (project.solutionSetup.originalSolution.solutionType == SolutionType.FINAL)
            f.writelnf("add_definitions(-DBUILD_FINAL)");
        else
            f.writelnf("add_definitions(-DBUILD_DEV)");

        f.writeln("# Project include directories");
        List<Path> sourceRoots = extractSourceRoots(project);
        sourceRoots.forEach( root -> f.writelnf("include_directories(%s)", project.solutionSetup.escapePath(root)));
        f.writeln("");

        f.writeln("# Project library includes");
        generateProjectLibrariesIncludes(project, f);
        f.writeln("");

        f.writeln("# Project files");
        List<String> additionalDependencies = new ArrayList<String>();
        List<File> allProjectFiles = getProjectFiles(project);
        allProjectFiles.forEach(pf -> generateProjectFileEntry(project, pf, f, additionalDependencies));
        f.writeln("");

        // get all source files
        if (requiresRTTI(project))
        {
            Path reflectionFile = project.localGeneratedPath.resolve("reflection.inl");

            f.writeln("# Generated reflection file");
            f.writelnf("list(APPEND FILE_HEADERS %s)", project.solutionSetup.escapePath(project.localGeneratedPath.resolve("reflection.inl")));
            f.writeln("");
        }

        f.writeln("# Project output");
        if (project.originalProject.attributes.hasKey("app")) {
            if (project.solutionSetup.originalSolution.platformType == PlatformType.WINDOWS || project.solutionSetup.originalSolution.platformType == PlatformType.UWP) {
                f.writelnf("add_executable(%s WIN32 ${FILE_SOURCES} ${FILE_HEADERS})", name);
            } else {
                f.writelnf("add_executable(%s ${FILE_SOURCES} ${FILE_HEADERS})", name);
            }
        } else if (project.originalProject.attributes.hasKey("console")) {
            f.writelnf("add_executable(%s ${FILE_SOURCES} ${FILE_HEADERS})", name);
        } else {
            if ((project.solutionSetup.originalSolution.platformType == PlatformType.WINDOWS || project.solutionSetup.originalSolution.platformType == PlatformType.UWP) && !project.solutionSetup.originalSolution.solutionType.buildAsLibs) {
                f.writelnf("add_library(%s SHARED ${FILE_SOURCES} ${FILE_HEADERS})", name);
            } else {
                f.writelnf("add_library(%s ${FILE_SOURCES} ${FILE_HEADERS})", name);
            }
        }
        f.writeln("");

        f.writeln("# Project dependencies");
        generateProjectDependencyEntries(project, f);
        additionalDependencies.stream().forEach(dep -> f.writelnf("add_dependencies(%s %s)", project.originalProject.mergedName, dep));
        f.writeln("");

        if (project.solutionSetup.originalSolution.platformType == PlatformType.LINUX) {
            f.writeln("# Hardcoded system libraries");
            f.writelnf("target_link_libraries(%s dl rt)", name);
        }

        if (project.solutionSetup.originalSolution.platformType == PlatformType.WINDOWS || project.solutionSetup.originalSolution.platformType == PlatformType.UWP) {
            f.writeln("# Precompiled header setup");

            allProjectFiles.stream().filter(pf -> pf.type == FileType.SOURCE).forEach(pf -> {
                String pch = pf.attributes.value("pch");
                boolean use = true;
                boolean generate = false;
                if (pch.equals("generate")) {
                    generate = true;
                } else if (pch.equals("disable") || pch.equals("disabled")) {
                    use = false;
                }

                if (generate)
                    f.writelnf("set_source_files_properties(%s PROPERTIES COMPILE_FLAGS \"/Ycbuild.h\")", project.solutionSetup.escapePath(pf.absolutePath));
                else if (use)
                    f.writelnf("set_source_files_properties(%s PROPERTIES COMPILE_FLAGS \"/Yubuild.h\")", project.solutionSetup.escapePath(pf.absolutePath));
            });

            //f.writelnf("add_precompiled_header(%s build.h FORCEINCLUDE)", name);
            //f.writelnf("set_target_properties(%s PROPERTIES COTIRE_CXX_PREFIX_HEADER_INIT \"build.h\")", name);
            //f.writelnf("cotire(%s)", name);
        }
        f.writeln("");
    }

    private void generateProjectDependencyEntries(CMakeSourcesProjectSetup project, GeneratedFile f) {
        List<ProjectSourcesSetup> deps = project.solutionSetup.collectSortedDependencies(project);

        deps.stream()
                .filter(dep -> dep != project)
                .forEach(dep -> {
                    f.writelnf("target_link_libraries(%s %s)", project.originalProject.mergedName, dep.originalProject.mergedName);
            });
    }

    private void generateProjectFileEntry(CMakeSourcesProjectSetup project, File pf, GeneratedFile f, List<String> additionalDependencies) {
        switch (pf.type) {

            case SOURCE: {
                f.writelnf("list(APPEND FILE_SOURCES %s)", project.solutionSetup.escapePath(pf.absolutePath));
                break;
            }

            case HEADER: {
                f.writelnf("list(APPEND FILE_HEADERS %s)", project.solutionSetup.escapePath(pf.absolutePath));
                break;
            }

            case PROTO: {
                String outputs = "";
                outputs += " \"" + project.localGeneratedPath + "/" + pf.coreName + ".pb.cc" + "\"";
                outputs += " \"" + project.localGeneratedPath + "/" + pf.coreName + ".pb.h" + "\"";

                f.writelnf("add_custom_command(OUTPUT %s COMMAND ${PROTOC_LOCATION} --proto_path=\"%s\" --cpp_out=\"%s\" \"%s\")",
                        outputs,
                        pf.absolutePath.getParent().toString(),
                        project.localGeneratedPath,
                        pf.absolutePath);

                f.writelnf("list(APPEND FILE_SOURCES \"%s/%s.pb.cc\")", project.localGeneratedPath, pf.coreName);
                f.writelnf("list(APPEND FILE_HEADERS \"%s/%s.pb.h\")", project.localGeneratedPath, pf.coreName);
                break;
            }

            case ANTLR: {
                String outputs = "";
                outputs += " \"" + project.localGeneratedPath + "/" + pf.coreName + "Lexer.h" + "\"";
                outputs += " \"" + project.localGeneratedPath + "/" + pf.coreName + "Lexer.cpp" + "\"";
                outputs += " \"" + project.localGeneratedPath + "/" + pf.coreName + "Parser.h" + "\"";
                outputs += " \"" + project.localGeneratedPath + "/" + pf.coreName + "Parser.cpp" + "\"";
                outputs += " \"" + project.localGeneratedPath + "/" + pf.coreName + "Listener.h" + "\"";

                f.writelnf("add_custom_command(OUTPUT %s COMMAND java -cp ${ANTLR4CPP_JAR_LOCATION} org.antlr.v4.Tool \"%s\" -o \"%s\" -Dlanguage=Cpp DEPENDS \"%s\")",
                        outputs,
                        pf.absolutePath,
                        project.localGeneratedPath,
                        pf.absolutePath);

                f.writelnf("list(APPEND FILE_SOURCES \"%s/%sLexer.cpp\")", project.localGeneratedPath, pf.coreName);
                f.writelnf("list(APPEND FILE_SOURCES \"%s/%sParser.cpp\")", project.localGeneratedPath, pf.coreName);
                f.writelnf("list(APPEND FILE_HEADERS \"%s/%sLexer.h\")", project.localGeneratedPath, pf.coreName);
                f.writelnf("list(APPEND FILE_HEADERS \"%s/%sParser.h\")", project.localGeneratedPath, pf.coreName);
                f.writelnf("list(APPEND FILE_HEADERS \"%s/%sListener.h\")", project.localGeneratedPath, pf.coreName);

                break;
            }
        }
    }

    private Set<Library> collectProjectPublicLibraries(CMakeSourcesProjectSetup project, boolean recursive) {
        Set<Dependency> deps = project.solutionSetup.collectProjectDependencies(project, recursive, DependencyType.PUBLIC_LIBRARY);
        return deps.stream()
                .map(dep -> dep.resolvedLibrary)
                .filter(dep -> (dep != null))
                .collect(Collectors.toSet());
    }

    private Set<Library> collectProjectPrivateLibraries(CMakeSourcesProjectSetup project, boolean recursive) {
        Set<Dependency> deps = project.solutionSetup.collectProjectDependencies(project, recursive, DependencyType.PRIVATE_LIBRARY);
        return deps.stream()
                .map(dep -> dep.resolvedLibrary)
                .filter(dep -> (dep != null))
                .collect(Collectors.toSet());
    }

    private List<Library> collectProjectInternalLibraries(CMakeSourcesProjectSetup project) {
        return Sets.union(
                collectProjectPublicLibraries(project, true),
                collectProjectPrivateLibraries(project, false))
                .stream()
                .sorted((o1,o2) -> o1.absolutePath.compareTo(o2.absolutePath))
                .collect(Collectors.toList());
    }

    @Override
    public void generateProjectLibrariesLinkage(ProjectSourcesSetup project, boolean hasStaticInitialization, GeneratedFile f)
    {
        // no header based linking
    }

    private void generateProjectLibrariesIncludes(CMakeSourcesProjectSetup project, GeneratedFile f) {
        List<Library> libs = collectProjectInternalLibraries(project);
        generateProjectLibrariesIncludes(project, libs, f);
    }

    private void generateProjectLibrariesIncludes(CMakeSourcesProjectSetup project, List<Library> libs, GeneratedFile f) {
        libs.forEach(lib -> generateProjectLibrariesIncludes(project, lib, f));
    }

    private void generateProjectLibrariesIncludes(CMakeSourcesProjectSetup project, Library lib, GeneratedFile f) {
        CMakeSolutionSetup solution = (CMakeSolutionSetup)project.solutionSetup;

        // for every platform/config generate the list of files to include
        solution.allConfigs.forEach(configName ->
                solution.allPlatforms.forEach(platformName ->
                        generateProjectLibraryIncludes(project, lib, platformName, configName, f)));

    }

    private static boolean shouldCopyFile(Path from, Path to) {
        if (!Files.exists(to))
            return true;

        try {
            FileTime fromTime = Files.getLastModifiedTime(from);
            FileTime toTime = Files.getLastModifiedTime(to);
            return fromTime.compareTo(toTime) != 0;
        }
        catch (IOException e) {
            return true;
        }
    }

    private void generateProjectLibraryIncludes(CMakeSourcesProjectSetup project, Library lib, String platformName, String configName, GeneratedFile f) {
        // get the library setup for given platform and config
        Config.MergedState setup = lib.getSetup(platformName, configName);
        if (!setup.getIncludePaths().isEmpty() || !setup.getLibPaths().isEmpty()) {

            // emit the library include paths into the project
            if (configName.toUpperCase().equals("RELEASE"))
                setup.getIncludePaths().forEach(path -> f.writelnf("include_directories(%s)", project.solutionSetup.escapePath(path)));

            // emit the library link paths into the project
            setup.getLibPaths().forEach(path -> {
                        if (Files.exists(path)) {
                            if (configName.toUpperCase().equals("DEBUG"))
                                f.writelnf("link_libraries(debug %s)", project.solutionSetup.escapePath(path));
                            else if (configName.toUpperCase().equals("RELEASE"))
                                f.writelnf("link_libraries(optimized %s)", project.solutionSetup.escapePath(path));
                        } else {
                            System.err.printf("Library file '%s' referenced by library '%s' manifest does not exist\n", path, lib.name);
                        }
                });
        }

        // copy files
        setup.getDeployFiles().forEach(path -> {
            if (Files.exists(path.sourcePath)) {
                Path publishPath = project.solutionSetup.rootPublishPath.resolve(String.format("%s", configName.toLowerCase()));
                Path targetPath = publishPath.resolve(path.targetPath);
                if (shouldCopyFile(path.sourcePath, targetPath)) {
                    System.out.printf("Copying file \"%s\" to \"%s\"\n", path.sourcePath, targetPath);
                    try {
                        Files.createDirectories(targetPath.getParent());
                        Files.copy(path.sourcePath, targetPath, StandardCopyOption.REPLACE_EXISTING);
                        Files.setLastModifiedTime(targetPath, Files.getLastModifiedTime(path.sourcePath));
                    }
                    catch (IOException e) {
                        System.err.printf("Failed to copy '%s' referenced by library '%s' into '%s'\n", path.sourcePath, lib.name, targetPath);
                    }
                }
            } else {
                System.err.printf("Deployed file '%s' referenced by library '%s' manifest does not exist\n", path.sourcePath, lib.name);
            }
        });

        // system libaries imports
        if (!setup.getSystemLibraries().isEmpty()) {
            f.writeln("# Link with system libraries");
            setup.getSystemLibraries().forEach(sl -> generateProjectLibraryImport(project, sl, f));
            f.writeln("");
        }
    }

    private void generateProjectLibraryImport(CMakeSourcesProjectSetup project, SystemLibrary sl, GeneratedFile f) {
        // emit the library import
        f.writelnf("find_package(%s)", sl.getName());

        if (!sl.getIncludeDirsNames().isEmpty())
            f.writelnf("include_directories(${%s})", sl.getIncludeDirsNames());

        if (!sl.getIncludeFile().isEmpty())
            f.writelnf("include(${%s})", sl.getIncludeFile());

        if (!sl.getLinkDirsNames().isEmpty())
            f.writelnf("link_directories(${%s})", sl.getLinkDirsNames());

        if (!sl.getLibraryFiles().isEmpty())
            f.writelnf("target_link_libraries(%s ${%s})", project.originalProject.mergedName, sl.getLibraryFiles());

        if (!sl.getDefinitionsName().isEmpty())
            f.writelnf("set( CMAKE_CXX_FLAGS  \"${CMAKE_CXX_FLAGS} ${%s}\")",   sl.getDefinitionsName());

        if (!sl.getFlags().isEmpty())
            f.writelnf("set( CMAKE_CXX_FLAGS  \"${CMAKE_CXX_FLAGS} %s\")",   sl.getFlags());
    }

    private void generateSolution(CMakeSolutionSetup solution, GeneratedFilesCollection files) {
        // create a writer
        GeneratedFile f = files.createFile(solution.solutionPath);

        f.writeln("# Boomer Engine v4");
        f.writeln("# Written by Tomasz \"Rex Dex\" Jonarski");
        f.writeln("# AutoGenerated file. Please DO NOT MODIFY.");
        f.writeln("");

        f.writeln("project(BoomerEngine)");
        f.writeln("");
        f.writeln("cmake_minimum_required(VERSION 2.8.10)");
        f.writeln("");

        //f.writeln("#SET(CMAKE_C_COMPILER /usr/bin/gcc)");
        //f.writeln("#SET(CMAKE_CXX_COMPILER /usr/bin/gcc)");
        //f.writeln("#set(CMAKE_DISABLE_IN_SOURCE_BUILD ON)");
        //f.writeln("#set(CMAKE_DISABLE_SOURCE_CHANGES  ON)");

        f.writeln("set(CMAKE_VERBOSE_MAKEFILE ON)");
        f.writeln("set(CMAKE_COLOR_MAKEFILE ON)");
        f.writeln("set(CMAKE_CONFIGURATION_TYPES \"Debug;Checked;Release\")");
        f.writeln("set(OpenGL_GL_PREFERENCE \"GLVND\")");
        f.writelnf("set(CMAKE_MODULE_PATH %s)", solution.escapePath(solution.buildScriptsPath));
        f.writelnf("set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY %s)", solution.escapePath(solution.rootPublishPath.toAbsolutePath().toString() + "/$<CONFIG>/lib"));
        f.writelnf("set(CMAKE_LIBRARY_OUTPUT_DIRECTORY %s)", solution.escapePath(solution.rootPublishPath.toAbsolutePath().toString() + "/$<CONFIG>/lib"));
        f.writelnf("set(CMAKE_RUNTIME_OUTPUT_DIRECTORY %s)", solution.escapePath(solution.rootPublishPath.toAbsolutePath().toString() + "/$<CONFIG>/bin"));

        //if (solution.platformType == PlatformType.WINDOWS)
            f.writeln("set_property(GLOBAL PROPERTY USE_FOLDERS ON)");

        f.writeln("");

        //f.writeln("# Internal build tools (Java powered)");
        //f.writelnf("set(ANTLR4CPP_JAR_LOCATION \"%s/antlr-4.7-complete.jar\")", solution.buildToolsPath);
        //f.writelnf("set(RTTI_JAR_LOCATION \"%s/rtti.jar\")", solution.buildToolsPath);
        //f.writelnf("set(PROTOC_LOCATION \"%s/protoc/bin/protoc\")", solution.buildToolsThirdpartyPath);
        //f.writeln("");

        f.writeln("#include(cotire)");
        f.writeln("#include(PrecompiledHeader)");
        f.writeln("include(OptimizeForArchitecture)"); // Praise OpenSource!
        f.writeln("");

        solution.cmakeProjects.forEach(p -> {
            f.writelnf("add_subdirectory(%s)", solution.escapePath(p.localProjectFilePath.getParent()));
            if (p.originalProject.group != null) {
                f.writelnf("set_target_properties(%s PROPERTIES FOLDER %s)", p.originalProject.mergedName, p.originalProject.group.shortName);
            }
        });


    }

}
