#include <iostream>
#include <thread>
#include <SFML/Graphics.hpp>

#include "analysis/FFT.h"
#include "audio/AudioPlayer.h"

int main()
{

	FFT fft(4096);
	float samples[4096];
	float freq = 1000.0f;
	float sampleRate = 48000.0f;

	for (int i = 0; i < 4096; i++) {
		samples[i] = std::sin(2.0f * std::numbers::pi * freq * i / sampleRate);
	}

	fft.process(samples);
	const auto& mags = fft.getMagnitudes();

	// find peak bin
	size_t peak = std::max_element(mags.begin(), mags.end()) - mags.begin();
	std::cout << "Peak bin: " << peak << "\n";
	std::cout << "Peak frequency: " << fft.binToFrequency(peak, sampleRate) << " Hz\n";

	/*
	AudioPlayer player("../../resources/sin_1khz.wav");
	player.play();
	player.waitUntilFinished();
	*/
}


void renderWindow() {
	sf::RenderWindow window( sf::VideoMode( { 200, 200 } ), "SFML works!" );
	sf::CircleShape shape( 100.f );
	shape.setFillColor( sf::Color::Green );

	while ( window.isOpen() )
	{
		while ( const std::optional event = window.pollEvent() )
		{
			if ( event->is<sf::Event::Closed>() )
				window.close();
		}

		window.clear();
		window.draw( shape );
		window.display();
	}
}