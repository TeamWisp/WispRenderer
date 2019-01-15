#pragma once

#include <chrono>

namespace profiling
{
	class Measurement
	{
	public:
		Measurement() = default;
		~Measurement() = default;

		void StartMeasurement() 
		{
			m_measurement_start = std::chrono::high_resolution_clock::now();
		};
		void EndMeasurement()
		{
			m_measurement_end = std::chrono::high_resolution_clock::now();
		};

		double GetElapsedTime()
		{
			m_measured = true;
			return (m_measurement_end - m_measurement_start).count() / 1000000.0;
		};

		bool HasBeenMeasured()
		{
			return m_measured;
		};

	private:
		std::chrono::high_resolution_clock::time_point m_measurement_start;
		std::chrono::high_resolution_clock::time_point m_measurement_end;

		bool m_measured = false;
	};

	extern Measurement resource_creation_measurement;
}