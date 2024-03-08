@Library('utils@main') _

node {

  stage('Checkout'){
    checkout scm
  }

  def COMMIT_AUTHOR_EMAIL = sh (returnStdout: true, script: "git --no-pager show -s --format='%ae'").trim();
  def EQUIVALENCE_CHECK = 1;
  def isChanged = false
  stage('Build and Push Docker Image') {
    if (isDependencyInstallerChanged(env.BRANCH_NAME)) {
      def commitHash = sh(script: 'git rev-parse HEAD', returnStdout: true)
      commitHash = commitHash.replaceAll(/[^a-zA-Z0-9-]/, '')

      isChanged = true
      DOCKER_IMAGE_TAG = pushCIImage(env.BRANCH_NAME, commitHash)
    }
  }

  try {
      timeout(time: 9, unit: 'HOURS') {  
        stage('Build and test') {
          Map tasks = [failFast: false]

          Map matrix_axes_tests = [
            TEST_SLUG: ["Unit tests",
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
                        "jpeg_sky130hs"]
          ]
          def axes = matrix_axes_tests.TEST_SLUG

          for (axisValue in axes) {
              def currentSlug = axisValue
              tasks["Local centos7 gcc - ${currentSlug}"] = {
                node {
                    checkout scm
                    // docker.image("openroad/ubuntu22.04-dev:${DOCKER_IMAGE_TAG}").inside('--user=root --privileged --rm -v /var/run/docker.sock:/var/run/docker.sock') {
                      // sh "git config --system --add safe.directory '*'"
                      try {
                        stage('Build centos7 gcc') {
                            sh './etc/Build.sh -no-warnings';
                        }
                        stage('Check message IDs') {
                            sh 'cd src && ../etc/find_messages.py > messages.txt';
                        }
                        stage('Test centos7 gcc') {
                            if ("${currentSlug}" == 'Unit tests') {
                              sh './test/regression'
                            }
                            else {
                              sh "./test/regression ${currentSlug}"
                            }
                        }
                      }
                      finally {
                          always {
                              sh "find . -name results -type d -exec tar zcvf {}.tgz {} ';'";
                              archiveArtifacts artifacts: '**/results.tgz', allowEmptyArchive: true;
                          }
                      }
                    // }
                }
              }
          }
          tasks["Local centos7 gcc without GUI"] = {
            node {
              checkout scm
              // docker.image("openroad/ubuntu22.04-dev:${DOCKER_IMAGE_TAG}").inside('--user=root --privileged --rm -v /var/run/docker.sock:/var/run/docker.sock') {
                stage('Build centos7 gcc without GUI') {
                  // sh "git config --system --add safe.directory '*'"
                  sh './etc/Build.sh -no-warnings -no-gui -dir=build-without-gui'; 
                }
              // }
            }
          }
          tasks["C++ Unit Tests"] = {
            node {
              checkout scm
              docker.image("openroad/ubuntu22.04-dev:${DOCKER_IMAGE_TAG}").inside('--user=root --privileged --rm -v /var/run/docker.sock:/var/run/docker.sock') {
                stage('C++ Unit Tests') {
                  sh "git config --system --add safe.directory '*'"
                  sh 'cmake -DCMAKE_C_COMPILER_LAUNCHER=ccache -DCMAKE_CXX_COMPILER_LAUNCHER=ccache -GNinja -B build .';
                  sh 'cd build && CLICOLOR_FORCE=1 ninja build_and_test';
                }
              }
            }
          }
          tasks["Test C++20 Compile"] = {
            node {
              checkout scm
              docker.image("openroad/ubuntu-cpp20").inside('--user=root --privileged --rm -v /var/run/docker.sock:/var/run/docker.sock') {
                stage('Test C++20 Compile') {
                  sh "git config --system --add safe.directory '*'"
                  sh "./etc/Build.sh -compiler='clang-16' -cmake='-DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_STANDARD=20'";
                }
              }
            }
          }
          Map matrix_axes = [
            OS: ['ubuntu20.04', 'ubuntu22.04'],
            COMPILER: ['gcc', 'clang'],
          ]

          for (os in matrix_axes.OS) {
            for (compiler in matrix_axes.COMPILER) {
              tasks["Docker ${os} ${compiler}"] = {
                node {
                  checkout scm
                  docker.image("openroad/ubuntu22.04-dev:${DOCKER_IMAGE_TAG}").inside('--user=root --privileged --rm -v /var/run/docker.sock:/var/run/docker.sock') {
                    stage("Pull ${os}") {
                      sh "git config --system --add safe.directory '*'"
                      retry(3) {
                        try {
                          sh "docker pull openroad/${os}-dev"
                        }
                        catch (err) {
                          echo err.getMessage();
                          sh 'sleep 1m ; exit 1';
                        }
                      }
                    }
                    stage("Build docker ${os}") {
                      sh "./etc/DockerHelper.sh create -os=${os} -target=builder -compiler=${compiler}"
                    }
                    stage("Test docker ${os}") {
                      sh "./etc/DockerHelper.sh test -os=${os} -target=builder -compiler=${compiler}"
                    }
                  }
                }
              }
            }
          }
          parallel(tasks)
        }
      }
    } finally {
      catchError(buildResult: 'FAILURE', stageResult: 'FAILURE') {
        sendEmail(env.BRANCH_NAME, COMMIT_AUTHOR_EMAIL, "", "OR")
      }
    }
}
