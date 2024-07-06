@Library('utils@or-v2.0.1') _

node {

    stage('Checkout'){
        checkout scm;
    }

    def DOCKER_IMAGE;
    stage('Build and Push Docker Image') {
        DOCKER_IMAGE = dockerPush("ubuntu22.04", "openroad");
        echo "Docker image is $DOCKER_IMAGE";
    }

    stage('Build and Stash bins') {
        buildBinsOR(DOCKER_IMAGE);
    }

    stage('Check message IDs') {
        dir('src') {
            sh '../etc/find_messages.py > messages.txt';
        }
        archiveArtifacts artifacts: 'src/messages.txt';
    }

    Map tasks = [failFast: false];

    tasks["Build without GUI"] = {
        node {
            checkout scm;
            stage('Build gcc without GUI') {
                sh './etc/Build.sh -no-gui -dir=build-without-gui';
            }
        }
    }

    tasks["Unit Tests CTest"] = {
        node {
            checkout scm;
            docker.image(DOCKER_IMAGE).inside('--user=root --privileged -v /var/run/docker.sock:/var/run/docker.sock') {
                unstash 'install';
                try {
                    catchError(buildResult: 'FAILURE', stageResult: 'FAILURE') {
                        sh 'ctest --test-dir build -j $(nproc)'
                    }
                }
                catch (e) {
                    echo "Failed regressions";
                    currentBuild.result = 'FAILURE';
                }
                finally {
                    sh "find . -name results -type d -exec tar zcvf {}.tgz {} ';'";
                    archiveArtifacts artifacts: '**/results.tgz', allowEmptyArchive: true;
                }
            }
        }
    }

    tasks["Unit Tests TCL"] = {
        node {
            checkout scm;
            docker.image(DOCKER_IMAGE).inside('--user=root --privileged -v /var/run/docker.sock:/var/run/docker.sock') {
                unstash 'install';
                try {
                    catchError(buildResult: 'FAILURE', stageResult: 'FAILURE') {
                        sh './test/regression'
                    }
                }
                catch (e) {
                    echo "Failed regressions";
                    currentBuild.result = 'FAILURE';
                }
                finally {
                    sh "find . -name results -type d -exec tar zcvf {}.tgz {} ';'";
                    archiveArtifacts artifacts: '**/results.tgz', allowEmptyArchive: true;
                }
            }
        }
    }

    tasks["C++ Unit Tests"] = {
        node {
            checkout scm;
            docker.image(DOCKER_IMAGE).inside('--user=root --privileged -v /var/run/docker.sock:/var/run/docker.sock') {
                stage('C++ Unit Tests Steup') {
                    sh 'cmake -DCMAKE_C_COMPILER_LAUNCHER=ccache -DCMAKE_CXX_COMPILER_LAUNCHER=ccache -GNinja -B build .';
                }
                stage('C++ Unit Tests Run') {
                    sh 'cd build && CLICOLOR_FORCE=1 ninja build_and_test';
                }
            }
        }
    }

    tasks["Test C++20 Compile"] = {
        node {
            checkout scm;
            docker.image("openroad/ubuntu-cpp20").inside('--user=root --privileged -v /var/run/docker.sock:/var/run/docker.sock') {
                stage('Test C++20 Compile') {
                    sh "git config --system --add safe.directory '*'";
                    sh "./etc/Build.sh -compiler='clang-16' -cmake='-DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_STANDARD=20'";
                }
            }
        }
    }

    def test_slugs = [
        "aes_nangate45",
        "gcd_nangate45",
        "tinyRocket_nangate45",
        "aes_sky130hd",
        "gcd_sky130hd",
        "ibex_sky130hd",
        "jpeg_sky130hd",
        "aes_sky130hs",
        "gcd_sky130hs",
        "ibex_sky130hs",
        "jpeg_sky130hs"
    ];

    test_slugs.each { test ->
        def currentSlug = test;
        tasks["Flow Test - ${currentSlug}"] = {
            node {
                checkout scm;
                docker.image(DOCKER_IMAGE).inside('--user=root --privileged -v /var/run/docker.sock:/var/run/docker.sock') {
                    unstash 'install';
                    try {
                        catchError(buildResult: 'FAILURE', stageResult: 'FAILURE') {
                            sh "./test/regression ${currentSlug}";
                        }
                    }
                    catch (e) {
                        echo "Failed regressions";
                        currentBuild.result = 'FAILURE';
                    }
                    finally {
                        sh "find . -name results -type d -exec tar zcvf {}.tgz {} ';'";
                        archiveArtifacts artifacts: '**/results.tgz', allowEmptyArchive: true;
                    }
                }
            }
        }
    }

    timeout(time: 2, unit: 'HOURS') {
        parallel(tasks);
    }

    stage('Send Email Report') {
        sendEmail();
    }

}
