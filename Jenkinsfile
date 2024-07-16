@Library('utils@or-v2.0.1') _

def baseTests(String image) {
    Map base_tests = [failFast: false];

    base_tests['Unit Tests CTest'] = {
        docker.image(image).inside('--user=root --privileged -v /var/run/docker.sock:/var/run/docker.sock') {
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
                archiveArtifacts artifacts: 'results-ctest.tgz';
            }
        }
    }

    base_tests['Unit Tests Tcl'] = {
        node {
            docker.image(image).inside('--user=root --privileged -v /var/run/docker.sock:/var/run/docker.sock') {
                stage('Setup Tcl Tests') {
                    sh label: 'Configure git', script: "git config --system --add safe.directory '*'";
                    checkout scm;
                    unstash 'install';
                }
                stage('Unit Tests TCL') {
                    try {
                        catchError(buildResult: 'FAILURE', stageResult: 'FAILURE') {
                            timeout(time: 20, unit: 'MINUTES') {
                                sh label: 'Tcl regression', script: './test/regression';
                            }
                        }
                    } catch (e) {
                        echo 'Failed regressions';
                        currentBuild.result = 'FAILURE';
                    }
                    sh label: 'Save Tcl results', script: "find . -name results -type d -exec tar zcvf {}.tgz {} ';'";
                    archiveArtifacts artifacts: '**/results.tgz';
                }
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
                docker.image(image).inside('--user=root --privileged -v /var/run/docker.sock:/var/run/docker.sock') {
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
                docker.image(image).inside('--user=root --privileged -v /var/run/docker.sock:/var/run/docker.sock') {
                    stage('Setup no-GUI Build') {
                        echo "Build without GUI";
                        sh label: 'Configure git', script: "git config --system --add safe.directory '*'";
                        checkout scm;
                    }
                    stage('no-GUI Build') {
                        timeout(time: 20, unit: 'MINUTES') {
                            sh label: 'no-GUI Build', script: './etc/Build.sh -no-gui -dir=build-without-gui';
                        }
                    }
                }
            }
        },

        'Build without Test': {
            node {
                docker.image(image).inside('--user=root --privileged -v /var/run/docker.sock:/var/run/docker.sock') {
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
                docker.image(image).inside('--user=root --privileged -v /var/run/docker.sock:/var/run/docker.sock') {
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
                buildBinsOR(image);
            }
            stage('Tests') {
                parallel(baseTests(image));
            }
        },

        'Compile with C++20': {
            node {
                docker.image('openroad/ubuntu-cpp20').inside('--user=root --privileged -v /var/run/docker.sock:/var/run/docker.sock') {
                    stage('Setup C++20 Compile') {
                        sh label: 'Configure git', script: "git config --system --add safe.directory '*'";
                        checkout scm;
                    }
                    stage('Compile with C++20') {
                        sh label: 'Compile C++20', script: "./etc/Build.sh -compiler='clang-16' -cmake='-DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_STANDARD=20'";
                    }
                }
            }
        }

    ];

    return ret;
}

timeout(time: 2, unit: 'HOURS') {
    node {
        stage('Checkout') {
            checkout scm;
        }
        def DOCKER_IMAGE;
        stage('Build and Push Docker Image') {
            DOCKER_IMAGE = dockerPush('ubuntu22.04', 'openroad');
            echo "Docker image is ${DOCKER_IMAGE}";
        }
        parallel(getParallelTests(DOCKER_IMAGE));
        stage('Send Email Report') {
            sendEmail();
        }
    }
}
