#include <iostream>
#include <thread>
#include <SFML/Graphics.hpp>

#include "analysis/AutoGain.h"
#include "analysis/FFT.h"
#include "analysis/SpectrumAnalyzer.h"
#include "audio/AudioPlayer.h"
#include "audio/FrequencyBand.h"
#include "util/MinMaxTracker.h"

static MinMaxTracker dbTracker;

void renderWindow(SpectrumAnalyzer& analyzer, float sampleRate, size_t fftLength) {
	sf::RenderWindow window(sf::VideoMode({1920, 1080}), "Spectrum Visualizer");
	window.setFramerateLimit(60);

	constexpr size_t numBars = 6;   // change freely now
	std::vector<FrequencyBand> bands = generateBands(numBars, 20, 20000);   // new

	std::vector<sf::RectangleShape> bars(numBars);
	std::vector<float> displayedHeights(numBars, 0.0f);
	AutoGain autoGain(numBars, 0.0f, -30.0f, 0.9f, 0.9f, 0.4);

	float windowWidth = 1920.0f;
	float windowHeight = 1080.0f;
	float centerY = windowHeight / 2.0f;
	float barWidth = (1920.0f / 1.5f) / numBars;   // changed — see below

	barWidth = std::min(50.0f, barWidth);

	std::vector<float> bandTilt(numBars);   // changed from fixed-size array — see below
	const float referenceHz = 750.0f;
	const float tiltPerOctave = 3.25f;

	float groupWidth = numBars * barWidth;
	float groupStartX = (windowWidth - groupWidth) / 2.0f;

	for (size_t i = 0; i < numBars; i++) {
		bars[i].setSize({barWidth - 4.0f, 0.0f});
		bars[i].setPosition({groupStartX + i * barWidth, centerY});
		bars[i].setFillColor(sf::Color::Cyan);
	}

	while (window.isOpen()) {
		while (const std::optional event = window.pollEvent()) {
			if (event->is<sf::Event::Closed>())
				window.close();
		}

		const auto& mags = analyzer.getMagnitudes();

		std::vector<float> bandDb(numBars);   // changed — see below
		const float maxBarHeight = windowHeight * 0.8f;

		for (size_t i = 0; i < numBars; i++) {
			float avg = averageBandMagnitude(mags, bands[i], sampleRate, fftLength);   // bands[i], not BANDS[i]
			float bandCenterHz = std::sqrt(bands[i].lowHz * bands[i].highHz);
			bandTilt[i] = tiltPerOctave * std::log2(bandCenterHz / referenceHz);
			bandDb[i] = magnitudeToDb(avg) + bandTilt[i];
		}

		std::vector<float> normalized(numBars);   // changed — see below
		autoGain.normalize(bandDb.data(), normalized.data(), numBars);

		for (size_t i = 0; i < numBars; i++) {
			float shaped = std::pow(normalized[i], 1.25f);
			float targetHeight = shaped * maxBarHeight;
			float rate = (targetHeight > displayedHeights[i]) ? 0.25f : 0.7f;

			displayedHeights[i] += rate * (targetHeight - displayedHeights[i]);
			if (displayedHeights[i] <= 10) displayedHeights[i] = 10;

			float halfHeight = displayedHeights[i] / 2.0f;
			bars[i].setSize({barWidth - 4.0f, displayedHeights[i]});
			bars[i].setPosition({groupStartX + i * barWidth + 2.0f, centerY - halfHeight});
		}

		window.clear(sf::Color::Black);
		for (auto& bar : bars) window.draw(bar);
		window.display();
	}
}

void runVisualizer() {
	AudioRingBuffer ringBuffer;
	AudioPlayer player("../../resources/sicko_mode.mp3", ringBuffer );
	SpectrumAnalyzer analyzer(ringBuffer);

	analyzer.start();
	player.play();

	renderWindow(analyzer, 48000.0f, 2048);
	player.waitUntilFinished();
	analyzer.stop();
}

int main()
{
	runVisualizer();
	for (float v : dbTracker.getTop())    std::cout << "high: " << v << "\n";
	for (float v : dbTracker.getBottom()) std::cout << "low: "  << v << "\n";
}
