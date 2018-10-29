pipeline {
    agent any
    stages {
        stage('Build') {
            steps {
                echo "Running ${env.BUILD_ID} on ${env.JENKINS_URL}"
				bat 'cd "%WORKSPACE%\\Scripts
				call JenkinsWebhook.bat ":bulb: Building pull request started. Jenkins build nr. %BUILD_NUMBER% started."
				cd "%WORKSPACE%
				cmake ./RayTracingLib/
				cmake --build ./RayTracingLib/ --target RayTracingLib
				mkdir builds
				move ./RayTracingLib/Debug "./builds/build_%BUILD_NUMBER%"'
				
				bat 'cd "%WORKSPACE%\\Scripts
				call "JenkinsWebhook.bat" ":white_check_mark: Jenkins build nr. %BUILD_NUMBER% succeeded!"'
				
				
            }
        }
    }
}