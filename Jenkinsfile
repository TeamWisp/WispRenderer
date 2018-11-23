pipeline {
    agent any
    stages {
        stage('Build') {
            steps {
                echo "Running ${env.BUILD_ID} on ${env.JENKINS_URL}"
				bat '''cd "%WORKSPACE%\\Scripts
				call JenkinsWebhook.bat ":bulb: Building Pull Request. Jenkins build nr: %BUILD_NUMBER%"
				cd "%WORKSPACE%
				install -j "%WORKSPACE%" 
				if errorlevel 1 (
					cd %WORKSPACE%\\Scripts 
					JenkinsWebhook ":x: Pull Request Build Failed!! Jenskins build nr: %BUILD_NUMBER% - install failed"
					EXIT 1
				)'''

				bat '''
				cd "%WORKSPACE%
				cmake --build ./build_vs2017_win32 
				if errorlevel 1 (
					cd %WORKSPACE%\\Scripts 
					JenkinsWebhook ":x: Pull Request Build Failed!! Jenskins build nr: %BUILD_NUMBER% - 32bit build failed"
				)
				cmake --build ./build_vs2017_win64 
				if errorlevel 1 (
					cd %WORKSPACE%\\Scripts 
					JenkinsWebhook ":x: Pull Request Build Failed!! Jenskins build nr: %BUILD_NUMBER% - 64bit build failed"
					EXIT 1
				)
				'''
				
				bat '''mkdir builds
				//move ./RayTracingLib/Debug "./builds/build_%BUILD_NUMBER%"
				cd "%WORKSPACE%\\scripts
				call "JenkinsWebhook.bat" ":white_check_mark: Pull Request Build Succesfull!! Jenkins build nr: %BUILD_NUMBER%"'''
            }
        }
    }
}
