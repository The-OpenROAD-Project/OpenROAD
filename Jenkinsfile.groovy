// pipeline {
//   agent any;
//   options {
//     timeout(time: 75, unit: 'MINUTES')
//   }
//   environment {
//     COMMIT_AUTHOR_EMAIL = sh (returnStdout: true, script: "git --no-pager show -s --format='%ae'").trim();
//     EQUIVALENCE_CHECK = 1;
//   }
//   stages {
//     stage('Build and test') {
//       parallel {
//         stage('Local centos7 gcc') {
//           agent any;
//           stages {
//             stage('Build centos7 gcc') {
//               steps {
//                 sh './etc/Build.sh -no-warnings';
//               }
//             }
//             stage('Check message IDs') {
//               steps {
//                 sh 'cd src && ../etc/find_messages.py > messages.txt';
//               }
//             }
//             stage('Test centos7 gcc') {
//               steps {
//                 script {
//                   parallel (
//                       'Unit tests':           { sh './test/regression' },
//                       'nangate45 aes':        { sh './test/regression aes_nangate45' },
//                       'nangate45 gcd':        { sh './test/regression gcd_nangate45' },
//                       'nangate45 tinyRocket': { sh './test/regression tinyRocket_nangate45' },
//                       'sky130hd aes':         { sh './test/regression aes_sky130hd' },
//                       'sky130hd gcd':         { sh './test/regression gcd_sky130hd' },
//                       'sky130hd ibex':        { sh './test/regression ibex_sky130hd' },
//                       'sky130hd jpeg':        { sh './test/regression jpeg_sky130hd' },
//                       'sky130hs aes':         { sh './test/regression aes_sky130hs' },
//                       'sky130hs gcd':         { sh './test/regression gcd_sky130hs' },
//                       'sky130hs ibex':        { sh './test/regression ibex_sky130hs' },
//                       'sky130hs jpeg':        { sh './test/regression jpeg_sky130hs' },
//                       )
//                 }
//               }
//             }
//           }
//           post {
//             always {
//               sh "find . -name results -type d -exec tar zcvf {}.tgz {} ';'";
//               archiveArtifacts artifacts: '**/results.tgz', allowEmptyArchive: true;
//             }
//           }
//         }
//         stage('Local centos7 gcc without GUI') {
//           agent any;
//           stages {
//             stage('Build centos7 gcc without GUI') {
//               steps {
//                 sh './etc/Build.sh -no-warnings -no-gui -dir=build-without-gui';
//               }
//             }
//           }
//         }
//         stage('Docker centos7 gcc') {
//           agent any;
//           stages{
//             stage('Pull centos7') {
//               steps {
//                 retry(3) {
//                   script {
//                     try {
//                       sh 'docker pull openroad/centos7-dev'
//                     }
//                     catch (err) {
//                       echo err.getMessage();
//                       sh 'sleep 1m ; exit 1';
//                     }
//                   }
//                 }
//               }
//             }
//             stage('Build docker centos7') {
//               steps {
//                 script {
//                   parallel (
//                       'build gcc':   { sh './etc/DockerHelper.sh create -os=centos7 -target=builder -compiler=gcc' },
//                       'build clang': { sh './etc/DockerHelper.sh create -os=centos7 -target=builder -compiler=clang' },
//                       )
//                 }
//               }
//             }
//             stage('Test docker centos7') {
//               steps {
//                 script {
//                   parallel (
//                       'test gcc':   { sh './etc/DockerHelper.sh test -os=centos7 -target=builder -compiler=gcc' },
//                       'test clang': { sh './etc/DockerHelper.sh test -os=centos7 -target=builder -compiler=clang' },
//                       )
//                 }
//               }
//             }
//           }
//         }
//         stage('Docker Ubuntu 20.04 gcc') {
//           agent any;
//           stages{
//             stage('Pull Ubuntu 20.04') {
//               steps {
//                 retry(3) {
//                   script {
//                     try {
//                       sh 'docker pull openroad/ubuntu20.04-dev'
//                     }
//                     catch (err) {
//                       echo err.getMessage();
//                       sh 'sleep 1m ; exit 1';
//                     }
//                   }
//                 }
//               }
//             }
//             stage('Build docker Ubuntu 20.04') {
//               steps {
//                 script {
//                   parallel (
//                       'build gcc':   { sh './etc/DockerHelper.sh create -os=ubuntu20.04 -target=builder -compiler=gcc' },
//                       'build clang': { sh './etc/DockerHelper.sh create -os=ubuntu20.04 -target=builder -compiler=clang' },
//                       )
//                 }
//               }
//             }
//             stage('Test docker Ubuntu 20.04') {
//               steps {
//                 script {
//                   parallel (
//                       'test gcc':    { sh './etc/DockerHelper.sh test -os=ubuntu20.04 -target=builder -compiler=gcc' },
//                       'test clang': { sh './etc/DockerHelper.sh test -os=ubuntu20.04 -target=builder -compiler=clang' },
//                       )
//                 }
//               }
//             }
//           }
//         }
//         stage('Docker Ubuntu 22.04 gcc') {
//           agent any;
//           stages{
//             stage('Pull Ubuntu 22.04') {
//               steps {
//                 retry(3) {
//                   script {
//                     try {
//                       sh 'docker pull openroad/ubuntu22.04-dev'
//                     }
//                     catch (err) {
//                       echo err.getMessage();
//                       sh 'sleep 1m ; exit 1';
//                     }
//                   }
//                 }
//               }
//             }
//             stage('Build docker Ubuntu 22.04') {
//               steps {
//                 script {
//                   parallel (
//                       'build gcc':   { sh './etc/DockerHelper.sh create -os=ubuntu22.04 -target=builder -compiler=gcc' },
//                       'build clang': { sh './etc/DockerHelper.sh create -os=ubuntu22.04 -target=builder -compiler=clang' },
//                       )
//                 }
//               }
//             }
//             stage('Test docker Ubuntu 22.04') {
//               steps {
//                 script {
//                   parallel (
//                       'test gcc':   { sh './etc/DockerHelper.sh test -os=ubuntu22.04 -target=builder -compiler=gcc' },
//                       'test clang': { sh './etc/DockerHelper.sh test -os=ubuntu22.04 -target=builder -compiler=clang' },
//                       )
//                 }
//               }
//             }
//           }
//         }
//       }
//     }
//   }
//   post {
//     failure {
//       script {
//         if ( env.BRANCH_NAME == 'master' ) {
//           echo('Main development branch: report to stakeholders and commit author.');
//           EMAIL_TO="$COMMIT_AUTHOR_EMAIL, \$DEFAULT_RECIPIENTS, cherry@parallaxsw.com";
//           REPLY_TO="$EMAIL_TO";
//         } else {
//           echo('Feature development branch: report only to commit author.');
//           EMAIL_TO="$COMMIT_AUTHOR_EMAIL";
//           REPLY_TO='$DEFAULT_REPLYTO';
//         }
//         emailext (
//             to: "$EMAIL_TO",
//             replyTo: "$REPLY_TO",
//             subject: '$DEFAULT_SUBJECT',
//             body: '$DEFAULT_CONTENT',
//             );
//       }
//     }
//   }
// }

node {

  stage('Checkout'){
    checkout scm
  }

  def COMMIT_AUTHOR_EMAIL = sh (returnStdout: true, script: "git --no-pager show -s --format='%ae'").trim();
  def EQUIVALENCE_CHECK = 1;

  try {
    timeout(time: 9, unit: 'HOURS') {  
      stage('Build and test') {
        Map tasks = [failFast: false]
        tasks["Local centos7 gcc"] = {
          node {
              checkout scm
              try {
                stage('Build centos7 gcc') {
                    sh './etc/Build.sh -no-warnings';
                }
                stage('Check message IDs') {
                    sh 'cd src && ../etc/find_messages.py > messages.txt';
                }
                stage('Test centos7 gcc') {
                    parallel (
                        'Unit tests':           { sh './test/regression' },
                        'nangate45 aes':        { sh './test/regression aes_nangate45' },
                        'nangate45 gcd':        { sh './test/regression gcd_nangate45' },
                        'nangate45 tinyRocket': { sh './test/regression tinyRocket_nangate45' },
                        'sky130hd aes':         { sh './test/regression aes_sky130hd' },
                        'sky130hd gcd':         { sh './test/regression gcd_sky130hd' },
                        'sky130hd ibex':        { sh './test/regression ibex_sky130hd' },
                        'sky130hd jpeg':        { sh './test/regression jpeg_sky130hd' },
                        'sky130hs aes':         { sh './test/regression aes_sky130hs' },
                        'sky130hs gcd':         { sh './test/regression gcd_sky130hs' },
                        'sky130hs ibex':        { sh './test/regression ibex_sky130hs' },
                        'sky130hs jpeg':        { sh './test/regression jpeg_sky130hs' },
                    )
                }
              }
              finally {
                  always {
                      sh "find . -name results -type d -exec tar zcvf {}.tgz {} ';'";
                      archiveArtifacts artifacts: '**/results.tgz', allowEmptyArchive: true;
                  }
              }
          }
        }
        tasks["Local centos7 gcc without GUI"] = {
          node {
            checkout scm
            stage('Build centos7 gcc without GUI') {
              sh './etc/Build.sh -no-warnings -no-gui -dir=build-without-gui'; 
            }
          }
        }
        Map matrix_axes = [
          OS: ['ubuntu20.04', 'ubuntu22.04', 'centos7'],
          COMPILER: ['gcc', 'clang'],
        ]

        for (os in matrix_axes.OS) {
          for (compiler in matrix_axes.COMPILER) {
            tasks["Docker ${os} ${compiler}"] = {
              node {
                checkout scm
                stage("Pull ${os}") {
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
        parallel(tasks)
      }
    }
  } finally {
    catchError(buildResult: 'FAILURE', stageResult: 'FAILURE') {
      //  if ( env.BRANCH_NAME == 'master' ) {
      //     echo('Main development branch: report to stakeholders and commit author.');
      //     EMAIL_TO="$COMMIT_AUTHOR_EMAIL, \$DEFAULT_RECIPIENTS, cherry@parallaxsw.com";
      //     REPLY_TO="$EMAIL_TO";
      //   } else {
      //     echo('Feature development branch: report only to commit author.');
      //     EMAIL_TO="$COMMIT_AUTHOR_EMAIL";
      //     REPLY_TO='$DEFAULT_REPLYTO';
      //   }
      //   emailext (
      //       to: "$EMAIL_TO",
      //       replyTo: "$REPLY_TO",
      //       subject: '$DEFAULT_SUBJECT',
      //       body: '$DEFAULT_CONTENT',
      //       );
      sendEmail(env.BRANCH_NAME, COMMIT_AUTHOR_EMAIL, "")   
    }
  }
}
