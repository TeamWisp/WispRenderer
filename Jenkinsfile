pipeline {
    agent any
    stages {
        stage('Build') {
            steps {
                echo "Running ${env.BUILD_ID} on ${env.JENKINS_URL}"
				bat 'cd "%WORKSPACE%\\Scripts'
				bat 'call 'JenkinsWebhook.bat ":bulb: Building pull request started. Jenkins build nr. %BUILD_NUMBER% started."'
				bat 'cd "%WORKSPACE%'
				bat 'cmake ./RayTracingLib/'
				bat 'cmake --build ./RayTracingLib/ --target RayTracingLib'
				bat 'mkdir builds'
				bat 'move ./RayTracingLib/Debug "./builds/build_%BUILD_NUMBER%"'
				
				bat 'cd "%WORKSPACE%\\Scripts'
				bat 'call "JenkinsWebhook.bat" ":white_check_mark: Jenkins build nr. %BUILD_NUMBER% succeeded!"'
				
				
            }
        }
    }
}