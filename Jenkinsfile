@Library('utils@or-v2.0.1') _

def baseTests(String image) {
    Map base_tests = [failFast: false];

    base_tests['Unit Tests CTest'] = {
        withDockerContainer(args: '-u root', image: image) {
            stage('Setup CTest') {
                echo 'Nothing to be done.';
            }
            stage('Unit Tests CTest') {
                try {
                    catchError(buildResult: 'FAILURE', stageResult: 'FAILURE') {
                        timeout(time: 10, unit: 'MINUTES') {
                            sh label: 'Run ctest', script: 'ctest --test-dir build -j $(nproc)';
                        }
                    }
                } catch (e) {
                    echo 'Failed regressions';
                    currentBuild.result = 'FAILURE';
                }
                sh label: 'Save ctest results', script: 'tar zcvf results-ctest.tgz build/Testing';
                sh label: 'Save results', script: "find . -name results -type d -exec tar zcvf {}.tgz {} ';'";
                archiveArtifacts artifacts: 'results-ctest.tgz, **/results.tgz';
            }
        }
    }

    def flow_tests = [
        'aes_nangate45',
        'gcd_nangate45',
        'tinyRocket_nangate45',
        'aes_sky130hd',
        'gcd_sky130hd',
        'ibex_sky130hd',
        'jpeg_sky130hd',
        'aes_sky130hs',
        'gcd_sky130hs',
        'ibex_sky130hs',
        'jpeg_sky130hs'
    ];

    flow_tests.each { current_test ->
        base_tests["Flow Test - ${current_test}"] = {
            node {
                withDockerContainer(args: '-u root', image: image) {
                    stage("Setup ${current_test}") {
                        sh label: 'Configure git', script: "git config --system --add safe.directory '*'";
                        checkout scm;
                        unstash 'install';
                    }
                    stage("Flow Test - ${current_test}") {
                        try {
                            catchError(buildResult: 'FAILURE', stageResult: 'FAILURE') {
                                timeout(time: 1, unit: 'HOURS') {
                                    sh label: "Flow ${current_test}", script: "./test/regression ${current_test}";
                                }
                            }
                        }
                        catch (e) {
                            echo 'Failed regressions';
                            currentBuild.result = 'FAILURE';
                        }
                        sh label: "Save ${current_test} results", script: "find . -name results -type d -exec tar zcvf ${current_test}.tgz {} ';'";
                        archiveArtifacts artifacts: "${current_test}.tgz";
                    }
                }
            }
        }
    }

    return base_tests;

}

def getParallelTests(String image) {

    def ret = [

        'Build without GUI': {
            node {
                withDockerContainer(args: '-u root', image: image) {
                    stage('Setup no-GUI Build') {
                        echo "Build without GUI";
                        sh label: 'Configure git', script: "git config --system --add safe.directory '*'";
                        checkout scm;
                    }
                    stage('no-GUI Build') {
                        timeout(time: 20, unit: 'MINUTES') {
                            sh label: 'no-GUI Build', script: './etc/Build.sh -no-warnings -no-gui -dir=build-without-gui';
                        }
                    }
                }
            }
        },

        'Build without Test': {
            node {
                withDockerContainer(args: '-u root', image: image) {
                    stage('Setup no-test Build') {
                        echo "Build without Tests";
                        sh label: 'Configure git', script: "git config --system --add safe.directory '*'";
                        checkout scm;
                    }
                    stage('no-test Build') {
                        timeout(time: 20, unit: 'MINUTES') {
                            catchError(buildResult: 'FAILURE', stageResult: 'FAILURE') {
                                sh label: 'no-test Build', script: 'cmake -B build_no_tests -D ENABLE_TESTS=OFF 2>&1 | tee no_test.log';
                            }
                        }
                        archiveArtifacts artifacts: 'no_test.log';
                    }
                }
            }
        },

        'Check message IDs': {
            dir('src') {
                sh label: 'Find duplicated message IDs', script: '../etc/find_messages.py > messages.txt';
                archiveArtifacts artifacts: 'messages.txt';
            }
        },

        'Unit Tests Ninja': {
            node {
                withDockerContainer(args: '-u root', image: image) {
                    stage('Setup Ninja Tests') {
                        sh label: 'Configure git', script: "git config --system --add safe.directory '*'";
                        checkout scm;
                    }
                    stage('C++ Unit Tests Setup') {
                        sh label: 'C++ Unit Tests Setup', script: 'cmake -DCMAKE_C_COMPILER_LAUNCHER=ccache -DCMAKE_CXX_COMPILER_LAUNCHER=ccache -GNinja -B build .';
                    }
                    stage('C++ Unit Tests') {
                        sh label: 'C++ Unit Tests', script: 'cd build && CLICOLOR_FORCE=1 ninja build_and_test';
                    }
                }
            }
        },

        'Build and Test': {
            stage('Build and Stash bins') {
                buildBinsOR(image, "-no-warnings");
            }
            stage('Tests') {
                parallel(baseTests(image));
            }
        },

        'Compile with C++20': {
            node {
                withDockerContainer(args: '-u root', image: image) {
                    stage('Setup C++20 Compile') {
                        sh label: 'Configure git', script: "git config --system --add safe.directory '*'";
                        checkout scm;
                    }
                    stage('Compile with C++20') {
                        sh label: 'Compile C++20', script: "./etc/Build.sh -no-warnings -compiler='clang-16' -cmake='-DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_STANDARD=20'";
                    }
                }
            }
        }

    ];

    if (env.BRANCH_NAME == 'master') {
        deb_os = [
            [name: 'Ubuntu 20.04' , artifact_name: 'ubuntu-20.04', image: 'openroad/ubuntu20.04-dev'],
            [name: 'Ubuntu 22.04' , artifact_name: 'ubuntu-22.04', image: 'openroad/ubuntu22.04-dev'],
            [name: 'Debian 11' , artifact_name: 'debian11', image: 'openroad/debian11-dev']
        ];
        deb_os.each { os ->
            ret["Build .deb - ${os.name}"] = {
                node {
                    stage('Setup and Build') {
                        sh label: 'Pull latest image', script: "docker pull ${os.image}:latest";
                        withDockerContainer(args: '-u root', image: "${os.image}") {
                            sh label: 'Configure git', script: "git config --system --add safe.directory '*'";
                            checkout([
                                    $class: 'GitSCM',
                                    branches: [[name: scm.branches[0].name]],
                                    doGenerateSubmoduleConfigurations: false,
                                    extensions: [
                                    [$class: 'CloneOption', noTags: false],
                                    [$class: 'SubmoduleOption', recursiveSubmodules: true]
                                    ],
                                    submoduleCfg: [],
                                    userRemoteConfigs: scm.userRemoteConfigs
                            ]);
                            def version = sh(script: 'git describe | sed s,^v,,', returnStdout: true).trim();
                            sh label: 'Create Changelog', script: "./debian/create-changelog.sh ${version}";
                            sh label: 'Run debuild', script: 'debuild --preserve-env --preserve-envvar=PATH -B -j$(nproc)';
                            sh label: 'Move generated files', script: "./debian/move-artifacts.sh ${version} ${os.artifact_name}";
                            archiveArtifacts artifacts: '*' + "${version}" + '*';
                        }
                    }
                }
            }
        }
    }

    return ret;
}

node {
    stage('Checkout') {
        checkout scm;
    }
    def DOCKER_IMAGE;
    stage('Build, Test and Push Docker Image') {
        Map build_docker_images  = [failFast: false];
        test_os = [
            [name: 'Ubuntu 20.04', base: 'ubuntu:20.04', image: 'ubuntu20.04'],
            [name: 'Ubuntu 22.04', base: 'ubuntu:22.04', image: 'ubuntu22.04'],
            [name: 'Ubuntu 24.04', base: 'ubuntu:24.04', image: 'ubuntu24.04'],
            [name: 'RockyLinux 9', base: 'rockylinux:9', image: 'rockylinux9'],
            [name: 'Debian 11', base: 'debian:11', image: 'debian11']
        ];
        test_os.each { os ->
            build_docker_images["Test Installer - ${os.name}"] = {
                node {
                    checkout scm;
                    sh label: 'Build Docker image', script: "./etc/DockerHelper.sh create -target=builder -os=${os.image}";
                    sh label: 'Test Docker image', script: "./etc/DockerHelper.sh test -target=builder -os=${os.image}";
                    dockerPush("${os.image}", 'openroad');
                }
            }
        }
        parallel(build_docker_images);
        DOCKER_IMAGE = dockerPush('ubuntu22.04', 'openroad');
        echo "Docker image is ${DOCKER_IMAGE}";
    }
    parallel(getParallelTests(DOCKER_IMAGE));
    stage('Send Email Report') {
        sendEmail();
    }
}
