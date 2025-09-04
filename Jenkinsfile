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
                            sh label: 'Run ctest', script: 'ctest --test-dir build -j $(nproc) --output-on-failure';
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
        'Docs Tester': {
            node {
                withDockerContainer(args: '-u root', image: image) {
                    stage('Setup Docs Test') {
                        echo "Setting up Docs Tester environment in ${image}";
                        sh label: 'Configure git', script: "git config --system --add safe.directory '*'";
                        checkout([
                            $class: 'GitSCM',
                            branches: [[name: scm.branches[0].name]],
                            extensions: [[$class: 'SubmoduleOption', recursiveSubmodules: true]],
                            userRemoteConfigs: scm.userRemoteConfigs
                        ]);
                    }
                    stage('Run Docs Tests') {
                        sh label: 'Build messages', script: 'python3 docs/src/test/make_messages.py';
                        sh label: 'Preprocess docs', script: 'cd docs && make preprocess -j$(nproc)';
                        sh label: 'Run Tcl syntax parser', script: 'python3 docs/src/test/man_tcl_params.py';
                        sh label: 'Run readme parser', script: 'cd docs && make clean && python3 src/test/readme_check.py';
                    }
                }
            }
        },

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
                                sh label: 'no-test Build', script: './etc/Build.sh -no-warnings -no-tests';
                            }
                        }
                        sh 'mv build/openroad_build.log no_test.log'
                        archiveArtifacts artifacts: 'no_test.log';
                    }
                }
            }
        },

        'Build on RHEL8': {
            node ('rhel8') {
                stage('Setup RHEL8 Build') {
                    checkout scm;
                }
                stage('Build on RHEL8') {
                    catchError(buildResult: 'FAILURE', stageResult: 'FAILURE') {
                        timeout(time: 20, unit: 'MINUTES') {
                            sh label: 'Build on RHEL8', script: './etc/Build.sh 2>&1 | tee rhel8-build.log';
                        }
                    }
                    archiveArtifacts artifacts: 'rhel8-build.log';
                }
                stage('Unit Tests CTest') {
                    catchError(buildResult: 'FAILURE', stageResult: 'FAILURE') {
                        timeout(time: 10, unit: 'MINUTES') {
                            sh label: 'Run ctest', script: 'ctest --test-dir build -j $(nproc) --output-on-failure';
                        }
                    }
                    sh label: 'Save ctest results', script: 'tar zcvf results-ctest-rhel8.tgz build/Testing';
                    sh label: 'Save results', script: "find . -name results -type d -exec tar zcvf {}.tgz {} ';'";
                    archiveArtifacts artifacts: 'results-ctest-rhel8.tgz, **/results.tgz';
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
                    stage('C++ Build and Unit Tests') {
                        sh label: 'C++ Build with Ninja', script: './etc/Build.sh -no-warnings -ninja';
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
                        sh label: 'Compile C++20', script: "./etc/Build.sh -cpp20"
                    }
                }
            }
        }
    ];

    return ret;
}

def bazelTest = {
    node {
        stage('Setup') {
            checkout scm;
            sh label: 'Setup Docker Image', script: 'docker build -f docker/Dockerfile.bazel -t openroad/bazel-ci .';
        }
        withDockerContainer(args: '-u root -v /var/run/docker.sock:/var/run/docker.sock', image: 'openroad/bazel-ci:latest') {
            stage('bazelisk test ...') {
                withCredentials([file(credentialsId: 'bazel-cache-sa', variable: 'GCS_SA_KEY')]) {
                    timeout(time: 120, unit: 'MINUTES') {
                        def cmd = 'bazelisk test --config=ci --show_timestamps --test_output=errors --curses=no --force_pic';
                        if (env.BRANCH_NAME != 'master') {
                            cmd += ' --remote_upload_local_results=false';
                        }
                        cmd += ' --google_credentials=$GCS_SA_KEY';
                        try {
                            sh label: 'Bazel Build', script: cmd + ' ...';
                        } catch (e) {
                            currentBuild.result = 'FAILURE';
                            sh label: 'Bazel Build (keep_going)', script: cmd + ' --keep_going ...';
                        }
                    }
                }
            }
        }
    }
}

def dockerTests = {
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
                    sh label: 'Test Docker image', script: "./etc/DockerHelper.sh test -target=builder -os=${os.image} -smoke";
                    dockerPush("${os.image}", 'openroad');
                }
            }
        }
        parallel(build_docker_images);
        DOCKER_IMAGE = dockerPush('ubuntu22.04', 'openroad');
        echo "Docker image is ${DOCKER_IMAGE}";
    }
    parallel(getParallelTests(DOCKER_IMAGE));
}

node {
    def isDefaultBranch = (env.BRANCH_NAME == 'master')
    def daysToKeep = '20';
    def numToKeep = (isDefaultBranch ? '-1' : '10');
    properties([
        buildDiscarder(logRotator(
            daysToKeepStr:         daysToKeep,
            artifactDaysToKeepStr: daysToKeep,
            numToKeepStr:          numToKeep,
            artifactNumToKeepStr:  numToKeep
        ))
    ]);
    parallel(
            "Bazel": bazelTest,
            "Docker Tests": dockerTests
    );
    stage('Send Email Report') {
        sendEmail();
    }
}
