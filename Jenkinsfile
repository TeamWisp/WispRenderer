pipeline {
    agent any
    stages {
        stage('Build') {
            steps {
                echo "Running ${env.BUILD_ID} on ${env.JENKINS_URL}"
				bat '''cd "%WORKSPACE%\\Scripts
				call JenkinsWebhook.bat ":bulb: Release Build %BUILD_NUMBER% started."
				cd "%WORKSPACE%
				cmake ./RayTracingLib/
				cmake --build ./RayTracingLib/ --target RayTracingLib
				mkdir builds
				move ./RayTracingLib/Debug "./builds/build_%BUILD_NUMBER%"'''
				bat '''cd "%WORKSPACE%\\Scripts
				call "JenkinsWebhook.bat" ":white_check_mark: Release Build %BUILD_NUMBER% succeeded!"'''
            }
        }
    }
}