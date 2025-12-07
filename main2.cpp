/**
 * Simulación de Física Estática: Plano Inclinado con Polea y Sube y Baja (Palanca)
 * INTEGRACION FINAL
 *
 * Modificaciones en SeesawSimulator:
 * 1. Distancias (distP1, distP2) son ahora números enteros.
 * 2. Se asegura que la "Respuesta Correcta" (Peso P2) tenga un máximo de 4 decimales.
 * 3. Se calcula y almacena la respuesta correcta (correctWeightP2) al inicio del juego.
 * 4. La comprobación de victoria usa la respuesta precalculada y la tolerancia EPSILON.
 * * Requiere SFML >= 2.5
 */

#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <iostream>
#include <vector>
#include <cmath>
#include <string>
#include <sstream>
#include <iomanip>
#include <functional>
#include <ctime>
#include <cstdlib>
#include <algorithm>

const float G = 9.8f;
const float PI = 3.14159265359f;
const float EPSILON = 0.5f; // Tolerancia para equilibrio

float toRad(float deg) { return deg * PI / 180.f; }

// ----------------- UI / Utility (Clases originales) -----------------
// ... (Tooltip, ForceArrow, InputBox, Button, Slider - sin cambios relevantes en estas clases)
// ... (Las definiciones de estas clases se mantienen igual que en el código anterior)

// ForceArrow (Mantengo la clase completa para referencia, aunque el cuerpo es el mismo)
class Tooltip {
// ... (Contenido de Tooltip)
private:
    sf::RectangleShape background;
    sf::Text textInfo;
    bool visible;
public:
    Tooltip(sf::Font& font) {
        background.setFillColor(sf::Color(50, 50, 50, 230));
        background.setOutlineColor(sf::Color::White);
        background.setOutlineThickness(1);

        textInfo.setFont(font);
        textInfo.setCharacterSize(14);
        textInfo.setFillColor(sf::Color::White);
        visible = false;
    }

    void show(std::string name, std::string formula, float value, sf::Vector2f pos, std::string extra = "") {
        std::stringstream ss;
        ss << std::fixed << std::setprecision(2);
        
        if (extra == "Seesaw") { 
            ss << "Momento: " << name << "\n";
            ss << "Formula: " << formula << "\n";
            ss << "Valor: " << value << " N*cm"; 
        } else { 
            ss << "Fuerza: " << name << "\n";
            ss << "Formula: " << formula << "\n";
            ss << "Valor: " << value << " N";
            if (!extra.empty()) ss << "\n(" << extra << ")";
        }
        

        textInfo.setString(ss.str());
        sf::FloatRect bounds = textInfo.getGlobalBounds();
        background.setSize(sf::Vector2f(bounds.width + 20, bounds.height + 20));
        background.setPosition(pos);
        textInfo.setPosition(pos.x + 10, pos.y + 10);
        visible = true;
    }

    void hide() { visible = false; }
    bool isVisible() const { return visible; }

    void draw(sf::RenderWindow& window) {
        if (visible) { window.draw(background); window.draw(textInfo); }
    }
};

class ForceArrow {
private:
    sf::RectangleShape line;
    sf::ConvexShape triangle;
    sf::Text label;
    std::string name;
    std::string formula;
    std::string extraDesc;
    float magnitude;
    bool isHovered;
    sf::Color baseColor;
public:
    ForceArrow(std::string n, std::string form, sf::Color c, sf::Font& font, std::string extra = "")
        : name(n), formula(form), baseColor(c), extraDesc(extra), isHovered(false), magnitude(0.0f) {
        triangle.setPointCount(3);
        label.setFont(font);
        label.setCharacterSize(12);
        label.setFillColor(sf::Color::Black);
        line.setFillColor(baseColor);
        triangle.setFillColor(baseColor);
    }

    void update(sf::Vector2f start, sf::Vector2f direction, float magValue, float scale) {
        magnitude = magValue;

        if (magValue < 0.05f) {
            line.setSize(sf::Vector2f(0, 0));
            triangle.setPosition(start);
            label.setString("");
            return;
        }

        float dirLen = std::sqrt(direction.x*direction.x + direction.y*direction.y);
        sf::Vector2f dirUnit = (dirLen > 0.0001f) ? (direction / dirLen) : sf::Vector2f(1,0);

        float vizLength = std::min(std::max(magValue * scale, 30.f), 160.f);

        line.setSize(sf::Vector2f(vizLength, 4));
        line.setOrigin(0, 2);
        line.setPosition(start);

        float angle = std::atan2(dirUnit.y, dirUnit.x) * 180.f / PI;
        line.setRotation(angle);

        triangle.setPoint(0, sf::Vector2f(0, 0));
        triangle.setPoint(1, sf::Vector2f(-10, -6));
        triangle.setPoint(2, sf::Vector2f(-10, 6));
        triangle.setPosition(start + dirUnit * vizLength);
        triangle.setRotation(angle);

        sf::Color drawColor = isHovered ? sf::Color(std::min(baseColor.r + 100, 255), std::min(baseColor.g + 100, 255), std::min(baseColor.b + 100, 255)) : baseColor;
        line.setFillColor(drawColor);
        triangle.setFillColor(drawColor);

        std::stringstream ss;
        ss << std::fixed << std::setprecision(1) << magValue << " N";
        label.setString(ss.str());
        label.setPosition(triangle.getPosition() + sf::Vector2f(10, 8));
    }

    bool checkHover(sf::Vector2f mousePos) {
        sf::FloatRect bounds = line.getGlobalBounds();
        bounds.width += 12; bounds.height += 12;
        bounds.left -= 6; bounds.top -= 6;
        isHovered = bounds.contains(mousePos);
        return isHovered;
    }

    void handleClick(sf::Vector2f mousePos, Tooltip& tooltip, std::string extra = "") {
        if (isHovered) tooltip.show(name, formula, magnitude, mousePos, extra);
    }

    void draw(sf::RenderWindow& window) {
        if (magnitude > 0.05f) {
            window.draw(line);
            window.draw(triangle);
            window.draw(label);
        }
    }
    
    float getMagnitude() const { return magnitude; }
    std::string getName() const { return name; }
    std::string getFormula() const { return formula; }
};

class InputBox {
// ... (Contenido de InputBox)
private:
    sf::RectangleShape box;
    sf::Text text;
    std::string currentString;
    bool hasFocus;
    float width, height;
public:
    InputBox(float x, float y, float w, float h, sf::Font& font) : width(w), height(h), hasFocus(false), currentString("") {
        box.setPosition(x, y);
        box.setSize(sf::Vector2f(w, h));
        box.setFillColor(sf::Color::White);
        box.setOutlineColor(sf::Color(100, 100, 100));
        box.setOutlineThickness(2);

        text.setFont(font);
        text.setCharacterSize(18);
        text.setFillColor(sf::Color::Black);
        text.setPosition(x + 5, y + 5);
    }

    void handleEvent(sf::Event event) {
        if (!hasFocus) return;
        if (event.type == sf::Event::TextEntered) {
            if (event.text.unicode == 8) {
                if (!currentString.empty()) currentString.pop_back();
            } else if ((event.text.unicode >= '0' && event.text.unicode <= '9') || event.text.unicode == '.') {
                if (event.text.unicode == '.' && currentString.find('.') != std::string::npos) return;
                
                if (currentString.length() < 8) currentString += static_cast<char>(event.text.unicode);
            }
            text.setString(currentString);
        }
    }

    void checkClick(sf::Vector2f mousePos) {
        if (box.getGlobalBounds().contains(mousePos)) { hasFocus = true; box.setOutlineColor(sf::Color::Blue); }
        else { hasFocus = false; box.setOutlineColor(sf::Color(100,100,100)); }
    }

    float getValue() const {
        if (currentString.empty()) return 0.0f;
        try { return std::stof(currentString); } catch (...) { return 0.0f; }
    }

    void clear() { currentString = ""; text.setString(""); }

    void setString(const std::string &s) { 
        currentString = s; 
        text.setString(currentString); 
        hasFocus = false; 
        box.setOutlineColor(sf::Color(100,100,100));
    }

    void draw(sf::RenderWindow& window) { window.draw(box); window.draw(text); }
};

class Button {
// ... (Contenido de Button)
private:
    sf::RectangleShape shape;
    sf::Text text;
    sf::Color baseColor;
public:
    Button(float x, float y, float w, float h, std::string label, sf::Font& font, sf::Color color) 
        : baseColor(color) {
        shape.setPosition(x, y);
        shape.setSize(sf::Vector2f(w, h));
        shape.setFillColor(color);

        text.setFont(font);
        text.setString(label);
        text.setCharacterSize(16);
        text.setFillColor(sf::Color::White);

        sf::FloatRect textRect = text.getLocalBounds();
        text.setOrigin(textRect.left + textRect.width/2.0f, textRect.top + textRect.height/2.0f);
        text.setPosition(x + w/2.0f, y + h/2.0f);
    }
    
    void setFillColor(sf::Color color) { 
        baseColor = color; 
        shape.setFillColor(color); 
    }
    
    const sf::Color& getFillColor() const { return baseColor; }

    bool isClicked(sf::Vector2f mousePos) const {
        return shape.getGlobalBounds().contains(mousePos);
    }

    void draw(sf::RenderWindow& window) const {
        window.draw(shape);
        window.draw(text);
    }
};

class Slider {
// ... (Contenido de Slider)
private:
    sf::RectangleShape bar;
    sf::CircleShape knob;
    float minVal, maxVal;
    float current;
    bool dragging;
    std::function<void(float)> valueChangedCallback; 

public:
    Slider(float x, float y, float w, float minV, float maxV, float initial = 0.0f, std::function<void(float)> callback = nullptr) 
        : minVal(minV), maxVal(maxV), current(initial), dragging(false), valueChangedCallback(callback) {
        
        bar.setPosition(x, y);
        bar.setSize(sf::Vector2f(w, 6));
        bar.setFillColor(sf::Color(180,180,180));

        knob.setRadius(9);
        knob.setOrigin(9, 9);
        knob.setFillColor(sf::Color(120,120,255));
        
        float t = (initial - minVal) / (maxVal - minVal);
        float kx = x + std::max(0.f, std::min(t * w, w));
        knob.setPosition(kx, y + 3);

        if (valueChangedCallback) {
            valueChangedCallback(current);
        }
    }

    void handleEvent(const sf::Event& e, sf::Vector2f mouse) {
        if (e.type == sf::Event::MouseButtonPressed) {
            if (knob.getGlobalBounds().contains(mouse)) dragging = true;
        } else if (e.type == sf::Event::MouseButtonReleased) {
            dragging = false;
        }
        if (dragging && (e.type == sf::Event::MouseMoved || e.type == sf::Event::MouseButtonPressed)) {
            float x = bar.getPosition().x;
            float w = bar.getSize().x;
            float newX = std::max(x, std::min(mouse.x, x + w));
            knob.setPosition(newX, knob.getPosition().y);
            float t = (newX - x) / w;
            
            float old_current = current; 
            current = minVal + t * (maxVal - minVal);
            
            if (current != old_current && valueChangedCallback) {
                valueChangedCallback(current); 
            }
        }
    }

    float getValue() const { return current; }

    void setValue(float v) {
        current = std::max(minVal, std::min(v, maxVal));
        float t = (current - minVal) / (maxVal - minVal);
        float x = bar.getPosition().x;
        float w = bar.getSize().x;
        float kx = x + t * w;
        knob.setPosition(kx, knob.getPosition().y);
        
        if (valueChangedCallback) {
            valueChangedCallback(current);
        }
    }

    void draw(sf::RenderWindow& window) {
        window.draw(bar);
        window.draw(knob);
    }
};


// ----------------- Física / Objetos (Clases originales) -----------------
class Block {
// ... (Contenido de Block)
public:
    sf::RectangleShape shape;
    std::vector<ForceArrow*> arrows;
    float mass;
    bool isOnSlope;

    Block(bool slope, sf::Color color, sf::Font& font) : isOnSlope(slope), mass(0) {
        shape.setSize(sf::Vector2f(50, 50));
        shape.setOrigin(25, 25);
        shape.setFillColor(color);
        shape.setOutlineColor(sf::Color::Black);
        shape.setOutlineThickness(2);

        if (isOnSlope) {
            arrows.push_back(new ForceArrow("Peso (W1)", "m1 * g", sf::Color::Red, font));
            arrows.push_back(new ForceArrow("Normal (N1)", "W1 * cos(\u03B8)", sf::Color::Blue, font));
            arrows.push_back(new ForceArrow("W Paralela", "W1 * sin(\u03B8)", sf::Color::Magenta, font));
            arrows.push_back(new ForceArrow("Friccion", "\u03bc * N1", sf::Color::Cyan, font));
            arrows.push_back(new ForceArrow("Tension", "T", sf::Color::Green, font));
        } else {
            arrows.push_back(new ForceArrow("Peso (W2)", "m2 * g", sf::Color::Red, font));
            arrows.push_back(new ForceArrow("Tension", "T", sf::Color::Green, font, "Tension cuerda = T"));
        }
    }

    ~Block() {
        for (auto a : arrows) delete a;
        arrows.clear();
    }

    void clearArrows() {
        for (auto &a : arrows) a->update(shape.getPosition(), sf::Vector2f(0,0), 0.0f, 1.0f);
    }

    void updatePhysics(float m, float thetaDeg, float tensionMag, float frictionMag, bool frictionUpSlope) {
        mass = m;
        float thetaRad = toRad(thetaDeg);
        float w = mass * G;
        float scale = 2.0f;

        if (isOnSlope) {
            sf::Vector2f downSlope(std::cos(thetaRad), std::sin(thetaRad));
            sf::Vector2f upSlope(-std::cos(thetaRad), -std::sin(thetaRad));
            sf::Vector2f normalDir(-std::sin(thetaRad), std::cos(thetaRad));
            sf::Vector2f gravityDir(0, 1);

            arrows[0]->update(shape.getPosition(), gravityDir, w, scale);
            arrows[1]->update(shape.getPosition(), normalDir, w * std::cos(thetaRad), scale);
            arrows[2]->update(shape.getPosition(), downSlope, w * std::sin(thetaRad), scale);

            sf::Vector2f fDir = frictionUpSlope ? upSlope : downSlope;
            arrows[3]->update(shape.getPosition(), fDir, frictionMag, scale);

            arrows[4]->update(shape.getPosition(), upSlope, tensionMag, scale);

        } else {
            arrows[0]->update(shape.getPosition(), sf::Vector2f(0,1), w, scale);
            arrows[1]->update(shape.getPosition(), sf::Vector2f(0,-1), tensionMag, scale);
        }
    }

    void draw(sf::RenderWindow& window) {
        window.draw(shape);
        for (auto a : arrows) a->draw(window);
    }

    void handleHover(sf::Vector2f mouse) {
        for (auto a : arrows) a->checkHover(mouse);
    }

    void handleClick(sf::Vector2f mouse, Tooltip& tooltip) {
        for (auto a : arrows) a->handleClick(mouse, tooltip);
    }
};

// ----------------- Clase Base para Simuladores -----------------
class SimulationBase {
protected:
    sf::Font& font;
    sf::Text msgLabel;
    Button* btnMenu;

public:
    SimulationBase(sf::Font& f) : font(f) {
        msgLabel.setFont(font);
        msgLabel.setCharacterSize(20);
        msgLabel.setFillColor(sf::Color::Black);

        btnMenu = new Button(800.f, 20.f, 180.f, 40.f, "Volver al Menu", font, sf::Color(100, 100, 100));
    }
    virtual ~SimulationBase() { delete btnMenu; }
    
    virtual int handleEvents(const sf::Event& event, sf::RenderWindow& window) = 0;
    virtual void update(sf::RenderWindow& window) = 0;
    virtual void draw(sf::RenderWindow& window) = 0;
    
    int checkMenuClick(sf::Vector2f mousePos) {
        if (btnMenu->isClicked(mousePos)) return 0; // 0 = Volver al Menú
        return -1; // -1 = No se hizo clic en el menú
    }
};

// ----------------- Simulador Nivel 1 (Clase original, adaptada para la estructura base) -----------------
class Simulator : public SimulationBase {
// ... (Contenido de Simulator)
private:
    InputBox* inputM1;
    InputBox* inputM2;
    InputBox* inputMu;
    Slider* sliderM1;
    Slider* sliderM2;
    Slider* sliderMu;
    Button* btnTest;
    Button* btnReset;
    Tooltip* tooltip;
    sf::Text labels[8];

    sf::ConvexShape ramp;
    sf::CircleShape pulley;
    sf::VertexArray rope;

    Block* blockYellow;
    Block* blockOrange;

    int currentAngle;
    int attempts;
    bool simulationActive;
    std::string message;
    float MU; 
    bool isWon;

public:
    Simulator(sf::Font& font) : SimulationBase(font), rope(sf::LineStrip), MU(0.2f), isWon(false) {
        setupUI();
        resetGame();
    }

    ~Simulator() override {
        delete inputM1;
        delete inputM2;
        delete inputMu;
        delete sliderM1;
        delete sliderM2;
        delete sliderMu;
        delete btnTest;
        delete btnReset;
        delete tooltip;
        delete blockYellow;
        delete blockOrange;
    }
    
    bool getIsWon() const { return isWon; }

    void updateInputFromSlider(InputBox* input, float value, int precision) {
        std::stringstream ss;
        ss << std::fixed << std::setprecision(precision) << value;
        input->setString(ss.str());
    }


    void setupUI() {
        float input_x = 50.f;
        float input_y1 = 50.f;
        float input_y2 = 140.f;
        float input_y3 = 230.f;
        float input_w = 100.f;
        float input_h = 30.f;
        float slider_offset = 40.f; 
        float slider_w = 200.f;
        
        inputM1 = new InputBox(input_x, input_y1, input_w, input_h, font);
        inputM2 = new InputBox(input_x, input_y2, input_w, input_h, font);
        inputMu = new InputBox(input_x, input_y3, input_w, input_h, font);

        sliderM1 = new Slider(input_x, input_y1 + slider_offset, slider_w, 0.5f, 100.0f, 5.0f, 
            [this](float v){ this->updateInputFromSlider(this->inputM1, v, 2); }); 
        
        sliderM2 = new Slider(input_x, input_y2 + slider_offset, slider_w, 0.5f, 100.0f, 5.0f, 
            [this](float v){ this->updateInputFromSlider(this->inputM2, v, 2); }); 
            
        sliderMu = new Slider(input_x, input_y3 + slider_offset, slider_w, 0.0f, 1.0f, 0.2f, 
            [this](float v){ this->updateInputFromSlider(this->inputMu, v, 3); }); 

        float button_x = 280.f;
        float button_y = 50.f;
        float message_y = 100.f;
        
        btnTest = new Button(button_x, button_y, 100, 30, "Probar", font, sf::Color(0,150,0));
        btnReset = new Button(button_x + 120, button_y, 100, 30, "Reiniciar", font, sf::Color(200,100,0));

        tooltip = new Tooltip(font);

        labels[0].setFont(font); labels[0].setString("Masa 1 (kg) (Amarillo):"); labels[0].setPosition(input_x, input_y1 - 25); labels[0].setCharacterSize(14); labels[0].setFillColor(sf::Color::Black);
        labels[1].setFont(font); labels[1].setString("Masa 2 (kg) (Naranja):"); labels[1].setPosition(input_x, input_y2 - 25); labels[1].setCharacterSize(14); labels[1].setFillColor(sf::Color::Black);
        labels[3].setFont(font); labels[3].setString("Coef. de friccion:"); labels[3].setPosition(input_x, input_y2 + 70); labels[3].setCharacterSize(14); labels[3].setFillColor(sf::Color::Black);
        
        msgLabel.setPosition(button_x, message_y);


        blockYellow = new Block(true, sf::Color::Yellow, font);
        blockOrange = new Block(false, sf::Color(255,165,0), font);
    }

    void resetGame() {
        std::vector<int> angles = {45, 30, 60, 16, 37, 53};
        currentAngle = angles[std::rand() % angles.size()];
        attempts = 3;
        simulationActive = false;
        isWon = false;
        message = "Intentos restantes: 3";
        msgLabel.setString(message);
        msgLabel.setFillColor(sf::Color::Black);

        sliderM1->setValue(5.0f);
        sliderM2->setValue(5.0f);
        sliderMu->setValue(0.2f);

        inputM1->clear(); inputM1->setString("5.00");
        inputM2->clear(); inputM2->setString("5.00");
        inputMu->clear(); inputMu->setString("0.00");
        MU = 0.2f;

        blockYellow->clearArrows();
        blockOrange->clearArrows();
        tooltip->hide();

        setupGeometry();
    }
    
    // ... (El resto de la lógica de Simulator es la misma)
    void setupGeometry() {
        float px = 400.0f;
        float py = 180.0f; 
        float base = 400.0f;
        float theta = toRad(currentAngle);
        float h = base * std::tan(theta);

        if (h > 350) { h = 350; base = h / std::tan(theta); }

        sf::Vector2f A(px, py);
        sf::Vector2f B(px, py + h);
        sf::Vector2f C(px + base, py + h);

        float a = 30.0f;
        if (currentAngle >= 60) a = 60.0f;
        sf::Vector2f A2(px, py + a);
        sf::Vector2f B2(px, py + a + h);
        sf::Vector2f C2(px + base, py + a + h);

        ramp.setPointCount(3);
        ramp.setPoint(0, A2);
        ramp.setPoint(1, B2);
        ramp.setPoint(2, C2);
        ramp.setFillColor(sf::Color(150,150,150));

        sf::Vector2f slopeDir = C - A;
        float len = std::sqrt(slopeDir.x*slopeDir.x + slopeDir.y*slopeDir.y);
        slopeDir /= len;

        float distOnSlope = 0.45f * len;
        sf::Vector2f blockYellowPos = A + slopeDir * distOnSlope;

        blockYellow->shape.setPosition(blockYellowPos);
        blockYellow->shape.setRotation(currentAngle);

        sf::Vector2f pulleyPos = A - slopeDir * 20.0f;
        pulley.setRadius(15);
        pulley.setOrigin(15,15);
        pulley.setPosition(pulleyPos);
        pulley.setFillColor(sf::Color(50,50,50));

        sf::Vector2f blockOrangePos(pulleyPos.x, pulleyPos.y + 120.0f);
        blockOrange->shape.setPosition(blockOrangePos);

        rope.clear();
        rope.setPrimitiveType(sf::LineStrip);
        rope.resize(3);
        rope[0].position = blockYellowPos;
        rope[1].position = pulleyPos;
        rope[2].position = blockOrangePos;
        rope[0].color = rope[1].color = rope[2].color = sf::Color::Black;
    }

    void calculatePhysics() {
        float m1 = inputM1->getValue();
        float m2 = inputM2->getValue();
        float inMu = inputMu->getValue();

        MU = inMu; 

        if (m1 <= 0 || m2 <= 0) {
            msgLabel.setString("Introduce masas validas (>0)");
            msgLabel.setFillColor(sf::Color::Red);
            return;
        }

        float thetaRad = toRad(currentAngle);
        float W1 = m1 * G;
        float W2 = m2 * G;
        float W1_para = W1 * std::sin(thetaRad);
        float N1 = W1 * std::cos(thetaRad);
        float Ff_max = MU * N1;
        float Tension = W2;

        float netForce = W1_para - W2;
        
        bool frictionUp = (netForce > 0); 
        
        float actualFriction = 0.0f;
        if (std::abs(netForce) < Ff_max + EPSILON) {
            actualFriction = std::abs(netForce);
            message = "EQUILIBRIO! GANASTE.";
            msgLabel.setFillColor(sf::Color::Green);
            simulationActive = true;
            isWon = true; 
        } else {
            actualFriction = Ff_max;

            attempts--;
            if (attempts > 0) {
                message = "No equilibrado. Intentos: " + std::to_string(attempts);
                msgLabel.setFillColor(sf::Color::Red);
            } else {
                message = "Juego terminado. Fallaste.";
                msgLabel.setFillColor(sf::Color::Red);
            }
            simulationActive = true;
            isWon = false;
        }


        blockYellow->updatePhysics(m1, currentAngle, Tension, actualFriction, frictionUp);
        blockOrange->updatePhysics(m2, 0, Tension, 0, false);
        
        msgLabel.setString(message);
    }
    
    int handleEvents(const sf::Event& event, sf::RenderWindow& window) override {
        sf::Vector2f mousePos = window.mapPixelToCoords(sf::Mouse::getPosition(window));

        if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
            int menu_status = checkMenuClick(mousePos);
            if (menu_status == 0) return 0;
        }

        inputM1->handleEvent(event);
        inputM2->handleEvent(event);
        inputMu->handleEvent(event);

        sliderM1->handleEvent(event, mousePos);
        sliderM2->handleEvent(event, mousePos);
        sliderMu->handleEvent(event, mousePos);

        if (event.type == sf::Event::MouseButtonPressed) {
            if (event.mouseButton.button == sf::Mouse::Left) {
                inputM1->checkClick(mousePos);
                inputM2->checkClick(mousePos);
                inputMu->checkClick(mousePos);

                if (btnReset->isClicked(mousePos)) resetGame();
                if (btnTest->isClicked(mousePos) && attempts > 0 && !isWon) calculatePhysics();

                tooltip->hide();
                if (simulationActive) {
                    blockYellow->handleClick(mousePos, *tooltip);
                    blockOrange->handleClick(mousePos, *tooltip);
                }
            } else {
                tooltip->hide();
            }
        }
        
        if (simulationActive) {
            blockYellow->handleHover(mousePos);
            blockOrange->handleHover(mousePos);
        }
        
        return 1; 
    }

    void update(sf::RenderWindow& window) override {} 
    
    void draw(sf::RenderWindow& window) override {
        window.clear(sf::Color(240,240,240));

        window.draw(ramp);
        if (rope.getVertexCount() > 0) window.draw(rope);
        window.draw(pulley);

        sf::Text angTxt;
        angTxt.setFont(font);
        angTxt.setString(std::to_string(currentAngle) + "\u00B0"); 
        angTxt.setPosition(ramp.getPoint(2).x - 60, ramp.getPoint(2).y - 30);
        angTxt.setFillColor(sf::Color::Black);
        angTxt.setCharacterSize(16);
        window.draw(angTxt);

        blockYellow->draw(window);
        blockOrange->draw(window);

        inputM1->draw(window);
        inputM2->draw(window);
        inputMu->draw(window);

        btnTest->draw(window);
        btnReset->draw(window);
        btnMenu->draw(window); 

        sliderM1->draw(window);
        sliderM2->draw(window);
        sliderMu->draw(window);

        for (int i = 0; i < 6; ++i) window.draw(labels[i]);
        window.draw(msgLabel); 

        tooltip->draw(window);
    }
};

// ----------------- Simulador Nivel 2 (NUEVA CLASE: Sube y Baja - MODIFICADA) -----------------

class SeesawSimulator : public SimulationBase {
private:
    // Geometría
    sf::RectangleShape board; 
    sf::CircleShape pivot;    
    sf::RectangleShape base;   
    
    // Figuras/Personas
    sf::CircleShape person1;
    sf::CircleShape person2;

    // UI
    InputBox* inputWeightP2;
    Button* btnCalculate;
    Button* btnNewGame;
    Tooltip* tooltip;
    
    // Flechas de fuerza (momento)
    ForceArrow* forceP1;
    ForceArrow* forceP2;

    // Parámetros de juego
    float weightP1, weightP2_input;
    int distP1, distP2; // <-- Ahora enteros (cm)
    float momentP1, momentP2;
    float correctWeightP2; // <-- Respuesta precalculada
    bool isWon;
    
    // Constantes Visuales
    const float BOARD_WIDTH = 600.f;
    const float BOARD_HEIGHT = 20.f;
    const float PIVOT_X = 500.f; 
    const float PIVOT_Y = 550.f; 

public:
    SeesawSimulator(sf::Font& font) : SimulationBase(font), isWon(false) {
        setupUI();
        setupGeometry();
        resetGame();
    }
    
    ~SeesawSimulator() override {
        delete inputWeightP2;
        delete btnCalculate;
        delete btnNewGame;
        delete tooltip;
        delete forceP1;
        delete forceP2;
    }
    
    bool getIsWon() const { return isWon; }

    // Generar un número entero aleatorio en el rango [min, max]
    int randomInt(int min, int max) {
        return min + (rand() % (max - min + 1));
    }
    
    // Generar un múltiplo de step en el rango [min, max] (para P1)
    int randomMultipleInt(int min, int max, int step) {
        int minStep = (min / step);
        int maxStep = (max / step);
        int randomStep = minStep + (rand() % (maxStep - minStep + 1));
        return randomStep * step;
    }

    /**
     * @brief Comprueba si un float tiene un máximo de 4 decimales significativos.
     * @details Se usa para regenerar los valores si la respuesta es demasiado compleja.
     * @param value El valor a comprobar (Peso P2 correcto).
     * @return true si el valor puede ser representado con 4 decimales o menos.
     */
    bool hasMaxFourDecimals(float value) {
        // Redondea el valor a 4 decimales
        float rounded = std::round(value * 10000.f) / 10000.f;
        // Comprueba si la diferencia es insignificante (menor a un error de 5ta decimal)
        return std::abs(value - rounded) < 0.000001f;
    }


    void setupUI() {
        float input_x = 50.f;
        float input_y = 50.f;
        float input_w = 120.f;
        float input_h = 30.f;
        float button_w = 180.f;
        float button_h = 40.f;
        
        inputWeightP2 = new InputBox(input_x, input_y + 30, input_w, input_h, font);

        btnCalculate = new Button(input_x, input_y + 80, button_w, button_h, "Calcular Equilibrio", font, sf::Color(0,100,180));
        btnNewGame = new Button(input_x, input_y + 130, button_w, button_h, "Nuevo Juego", font, sf::Color(200,100,0));
        
        sf::Text labelInput;
        labelInput.setFont(font); labelInput.setString("Peso P2 (kg):"); labelInput.setPosition(input_x, input_y); labelInput.setCharacterSize(18);
        
        msgLabel.setPosition(input_x, input_y + 190);
        msgLabel.setCharacterSize(22);
        
        tooltip = new Tooltip(font);
        
        forceP1 = new ForceArrow("Momento P1", "Peso * Distancia", sf::Color::Red, font, "Seesaw");
        forceP2 = new ForceArrow("Momento P2", "Peso * Distancia", sf::Color::Blue, font, "Seesaw");
    }

    void setupGeometry() {
        base.setSize(sf::Vector2f(20.f, 100.f));
        base.setOrigin(10.f, 0.f);
        base.setPosition(PIVOT_X, PIVOT_Y);
        base.setFillColor(sf::Color(100, 100, 100));
        
        pivot.setRadius(15.f);
        pivot.setOrigin(15.f, 15.f);
        pivot.setPosition(PIVOT_X, PIVOT_Y);
        pivot.setFillColor(sf::Color::Black);

        board.setSize(sf::Vector2f(BOARD_WIDTH, BOARD_HEIGHT));
        board.setOrigin(BOARD_WIDTH / 2.f, BOARD_HEIGHT / 2.f);
        board.setPosition(PIVOT_X, PIVOT_Y - BOARD_HEIGHT / 2.f);
        board.setFillColor(sf::Color(139, 69, 19)); 
        
        person1.setRadius(20.f); person1.setOrigin(20.f, 20.f); person1.setFillColor(sf::Color::Yellow); person1.setOutlineColor(sf::Color::Black); person1.setOutlineThickness(2.f);
        person2.setRadius(20.f); person2.setOrigin(20.f, 20.f); person2.setFillColor(sf::Color::Cyan); person2.setOutlineColor(sf::Color::Black); person2.setOutlineThickness(2.f);
    }
    
    void resetGame() {
        // Lógica de generación con comprobación de decimales en la respuesta
        do {
            // P1: Peso [50, 120] kg
            weightP1 = randomInt(50, 120);
            // P1: Distancia múltiplo de 20 en [20, 100] cm
            distP1 = randomMultipleInt(20, 100, 20);
            
            // P2: Distancia [10, 100] cm
            distP2 = randomInt(10, 100);

            // Cálculo de la respuesta correcta: Peso₂ = (Peso₁ * Distancia₁) / Distancia₂
            correctWeightP2 = (weightP1 * distP1) / (float)distP2;
            
        } while (!hasMaxFourDecimals(correctWeightP2));

        // UI y estado
        inputWeightP2->clear();
        isWon = false;
        weightP2_input = 0.f;
        momentP1 = 0.f;
        momentP2 = 0.f;
        
        std::stringstream ss;
        ss << std::fixed << std::setprecision(2) << correctWeightP2;

        //msgLabel.setString("Ingresa el peso de P2 para equilibrar.\nRespuesta requerida: " + ss.str() + " kg"); // Mostrar la respuesta para propósitos de prueba
        msgLabel.setFillColor(sf::Color::Black);
        
        forceP1->update(sf::Vector2f(0,0), sf::Vector2f(0,0), 0.f, 1.f);
        forceP2->update(sf::Vector2f(0,0), sf::Vector2f(0,0), 0.f, 1.f);

        updateVisualState();
    }
    
    void calculateEquilibrium() {
        weightP2_input = inputWeightP2->getValue();
        
        if (weightP2_input <= 0.01f) {
            msgLabel.setString("¡Ingresa un peso valido para P2!");
            msgLabel.setFillColor(sf::Color::Red);
            updateVisualState();
            return;
        }
        
        momentP1 = weightP1 * distP1;
        momentP2 = weightP2_input * distP2;
        
        // Criterio de Equilibrio: Peso de usuario vs. Peso correcto (precalculado)
        // Usamos la tolerancia para aceptar respuestas cercanas, incluyendo redondeo.
        float inputRatio = weightP2_input / correctWeightP2;
        
        if (std::abs(inputRatio - 1.0f) <= (EPSILON / correctWeightP2)) { // Normalizar la tolerancia
            // ¡EQUILIBRIO!
            isWon = true;
            msgLabel.setString("¡EQUILIBRIO LOGRADO! GANASTE.");
            msgLabel.setFillColor(sf::Color::Green);
        } else {
            // DESEQUILIBRIO
            isWon = false;
            msgLabel.setString("DESEQUILIBRIO: Ingresa otro peso.");
            msgLabel.setFillColor(sf::Color::Red);
        }

        updateVisualState();
    }

    void updateVisualState() {
        float max_angle = 15.f; 
        float max_moment = 120.f * 100.f; 
        float angle = 0.f;
        
        if (!isWon) {
            float moment_diff = momentP1 - momentP2; 
            angle = moment_diff / max_moment * max_angle;
            angle = std::max(-max_angle, std::min(max_angle, angle));
        }
        
        board.setRotation(angle);
        
        float rad_angle = toRad(angle);
        float y_offset = PIVOT_Y - BOARD_HEIGHT / 2.f;
        
        // P1 (izquierda, Distancia = distP1)
        float p1_dist_from_center = -(float)distP1 * BOARD_WIDTH / 200.f; 
        float p1_x = PIVOT_X + p1_dist_from_center * std::cos(rad_angle);
        float p1_y = y_offset + p1_dist_from_center * std::sin(rad_angle);
        
        // P2 (derecha, Distancia = distP2)
        float p2_dist_from_center = (float)distP2 * BOARD_WIDTH / 200.f; 
        float p2_x = PIVOT_X + p2_dist_from_center * std::cos(rad_angle);
        float p2_y = y_offset + p2_dist_from_center * std::sin(rad_angle);

        person1.setPosition(p1_x, p1_y - person1.getRadius());
        person2.setPosition(p2_x, p2_y - person2.getRadius());
        
        if (weightP2_input > 0.01f) {
            float scale_factor = 0.01f; 
            
            forceP1->update(person1.getPosition() + sf::Vector2f(0, 20), sf::Vector2f(0, 1), weightP1 * G, scale_factor);
            forceP2->update(person2.getPosition() + sf::Vector2f(0, 20), sf::Vector2f(0, 1), weightP2_input * G, scale_factor);
        } else {
            forceP1->update(sf::Vector2f(0,0), sf::Vector2f(0,0), 0.f, 1.f);
            forceP2->update(sf::Vector2f(0,0), sf::Vector2f(0,0), 0.f, 1.f);
        }
    }
    
    void drawData(sf::RenderWindow& window) {
        float x_left_p1 = PIVOT_X - (float)distP1 * BOARD_WIDTH / 200.f;
        float x_right_p2 = PIVOT_X + (float)distP2 * BOARD_WIDTH / 200.f;
        float y_dist = PIVOT_Y + 50.f;

        drawDashedLine(window, PIVOT_X, PIVOT_Y, x_left_p1, y_dist, sf::Color::Black);
        drawDashedLine(window, PIVOT_X, y_dist, x_left_p1, y_dist, sf::Color::Blue);
        
        sf::Text distTxt1;
        distTxt1.setFont(font); distTxt1.setCharacterSize(14); distTxt1.setFillColor(sf::Color::Blue);
        std::stringstream ss1; ss1 << std::fixed << std::setprecision(0) << (float)distP1 << " cm";
        distTxt1.setString(ss1.str());
        distTxt1.setPosition(PIVOT_X - (float)distP1 * BOARD_WIDTH / 400.f - 20, y_dist + 5);
        window.draw(distTxt1);

        drawDashedLine(window, PIVOT_X, PIVOT_Y, x_right_p2, y_dist, sf::Color::Black);
        drawDashedLine(window, PIVOT_X, y_dist, x_right_p2, y_dist, sf::Color::Red);
        
        sf::Text distTxt2;
        distTxt2.setFont(font); distTxt2.setCharacterSize(14); distTxt2.setFillColor(sf::Color::Red);
        std::stringstream ss2; ss2 << std::fixed << std::setprecision(0) << (float)distP2 << " cm";
        distTxt2.setString(ss2.str());
        distTxt2.setPosition(PIVOT_X + (float)distP2 * BOARD_WIDTH / 400.f - 20, y_dist + 5);
        window.draw(distTxt2);
        
        float data_x = 700.f;
        float data_y = 50.f;
        float line_spacing = 25.f;
        
        auto drawDataLine = [&](float y, const std::string& label, const std::string& value, sf::Color color) {
            sf::Text labelTxt; labelTxt.setFont(font); labelTxt.setCharacterSize(16); labelTxt.setFillColor(sf::Color::Black);
            labelTxt.setString(label); labelTxt.setPosition(data_x, y); window.draw(labelTxt);
            
            sf::Text valueTxt; valueTxt.setFont(font); valueTxt.setCharacterSize(16); valueTxt.setFillColor(color);
            valueTxt.setString(value); valueTxt.setPosition(data_x + 150, y); window.draw(valueTxt);
        };
        
        std::stringstream ssW1, ssW2, ssM1, ssM2;
        ssW1 << std::fixed << std::setprecision(0) << weightP1 << " kg";
        ssW2 << std::fixed << std::setprecision(2) << weightP2_input << " kg (Input)";
        ssM1 << std::fixed << std::setprecision(2) << momentP1 << " N*cm";
        ssM2 << std::fixed << std::setprecision(2) << momentP2 << " N*cm";

        drawDataLine(data_y + 0*line_spacing, "P1 Peso:", ssW1.str(), sf::Color::Yellow);
        drawDataLine(data_y + 1*line_spacing, "P1 Distancia:", std::to_string(distP1) + " cm", sf::Color::Blue);
        drawDataLine(data_y + 2*line_spacing, "P1 Momento:", ssM1.str(), sf::Color::Red);
        
        drawDataLine(data_y + 4*line_spacing, "P2 Peso:", ssW2.str(), sf::Color::Cyan);
        drawDataLine(data_y + 5*line_spacing, "P2 Distancia:", std::to_string(distP2) + " cm", sf::Color::Red);
        drawDataLine(data_y + 6*line_spacing, "P2 Momento:", ssM2.str(), sf::Color::Blue);
    }
    
    void drawDashedLine(sf::RenderWindow& window, float x1, float y1, float x2, float y2, sf::Color color) {
        const float segment_length = 5.f;
        const float gap_length = 3.f;

        float dx = x2 - x1;
        float dy = y2 - y1;
        float length = std::sqrt(dx*dx + dy*dy);
        
        if (length < segment_length) return;

        sf::Vector2f dir_unit(dx / length, dy / length);

        float current_pos = 0;
        while (current_pos < length) {
            float line_start = current_pos;
            float line_end = std::min(current_pos + segment_length, length);

            sf::Vertex line[] = {
                sf::Vertex(sf::Vector2f(x1 + dir_unit.x * line_start, y1 + dir_unit.y * line_start), color),
                sf::Vertex(sf::Vector2f(x1 + dir_unit.x * line_end, y1 + dir_unit.y * line_end), color)
            };
            window.draw(line, 2, sf::Lines);

            current_pos += segment_length + gap_length;
        }
    }
    
    int handleEvents(const sf::Event& event, sf::RenderWindow& window) override {
        sf::Vector2f mousePos = window.mapPixelToCoords(sf::Mouse::getPosition(window));

        if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
            int menu_status = checkMenuClick(mousePos);
            if (menu_status == 0) return 0; 
        }

        inputWeightP2->handleEvent(event);

        if (event.type == sf::Event::MouseButtonPressed) {
            if (event.mouseButton.button == sf::Mouse::Left) {
                inputWeightP2->checkClick(mousePos);

                if (btnNewGame->isClicked(mousePos)) resetGame();
                if (btnCalculate->isClicked(mousePos)) calculateEquilibrium();
                
                tooltip->hide();
                
                if (momentP1 > 0 || momentP2 > 0) {
                    if (forceP1->checkHover(mousePos)) {
                        tooltip->show(forceP1->getName(), forceP1->getFormula(), momentP1, mousePos, "Seesaw");
                    } else if (forceP2->checkHover(mousePos)) {
                        tooltip->show(forceP2->getName(), forceP2->getFormula(), momentP2, mousePos, "Seesaw");
                    }
                }
            }
        }
        
        if (momentP1 > 0 || momentP2 > 0) {
            forceP1->checkHover(mousePos);
            forceP2->checkHover(mousePos);
        }

        return 2; 
    }

    void update(sf::RenderWindow& window) override {
        // Nada
    }

    void draw(sf::RenderWindow& window) override {
        window.clear(sf::Color::White);
        
        inputWeightP2->draw(window);
        btnCalculate->draw(window);
        btnNewGame->draw(window);
        btnMenu->draw(window);
        
        sf::Text labelInput;
        labelInput.setFont(font); labelInput.setString("Peso P2 (kg):"); labelInput.setPosition(50.f, 50.f); labelInput.setCharacterSize(18); labelInput.setFillColor(sf::Color::Black);
        window.draw(labelInput);
        window.draw(msgLabel);
        
        drawData(window); // Dibujar datos primero para que no tapen los elementos centrales

        window.draw(base);
        window.draw(board);
        window.draw(pivot);
        
        window.draw(person1);
        window.draw(person2);
        
        forceP1->draw(window);
        forceP2->draw(window);

        tooltip->draw(window);
    }
};

// ----------------- Menú y Manejador Principal -----------------
enum class GameState { Menu, Level1, Level2 };

class GameMenu {
// ... (Contenido de GameMenu)
private:
    sf::RectangleShape background;
    Button* btnLevel1;
    Button* btnLevel2;
    sf::Font& font;

public:
    GameMenu(sf::Font& f) : font(f) {
        background.setFillColor(sf::Color::White);
        background.setSize(sf::Vector2f(1000, 700)); 
        
        float btn_w = 250.f;
        float btn_h = 80.f;
        float center_x = 500.f;
        float center_y = 350.f;

        btnLevel1 = new Button(center_x - btn_w - 20, center_y - btn_h/2, btn_w, btn_h, "NIVEL 1: Plano Inclinado", font, sf::Color(150, 150, 150));
        btnLevel2 = new Button(center_x + 20, center_y - btn_h/2, btn_w, btn_h, "NIVEL 2: Sube y Baja", font, sf::Color(150, 150, 150));
    }

    ~GameMenu() {
        delete btnLevel1;
        delete btnLevel2;
    }

    GameState handleEvent(const sf::Event& event, sf::Vector2f mousePos) {
        if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
            if (btnLevel1->isClicked(mousePos)) return GameState::Level1;
            if (btnLevel2->isClicked(mousePos)) return GameState::Level2;
        }
        return GameState::Menu;
    }

    void update(bool won1, bool won2) {
        btnLevel1->setFillColor(won1 ? sf::Color::Green : sf::Color(150, 150, 150));
        btnLevel2->setFillColor(won2 ? sf::Color::Green : sf::Color(150, 150, 150));
    }

    void draw(sf::RenderWindow& window) {
        window.draw(background);
        btnLevel1->draw(window);
        btnLevel2->draw(window);
        
        sf::Text title;
        title.setFont(font);
        title.setString("Simulador de Estática");
        title.setCharacterSize(40);
        title.setFillColor(sf::Color::Black);
        
        sf::FloatRect bounds = title.getLocalBounds();
        title.setOrigin(bounds.left + bounds.width/2.0f, bounds.top + bounds.height/2.0f);
        title.setPosition(500.f, 150.f);
        window.draw(title);
    }
};


int main() {
    std::srand(static_cast<unsigned int>(std::time(nullptr)));
    sf::RenderWindow window(sf::VideoMode(1000, 700), "Simulacion Estatica");
    window.setFramerateLimit(60);

    sf::Font font;
    if (!font.loadFromFile("src/arial.ttf")) {
        if (!font.loadFromFile("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf")) {
            std::cerr << "Error: No se encontro arial.ttf" << std::endl;
            return -1;
        }
    }

    Simulator level1(font);
    SeesawSimulator level2(font);
    GameMenu menu(font);

    GameState currentState = GameState::Menu;
    bool level1Won = false;
    bool level2Won = false;

    while (window.isOpen()) {
        sf::Event event;
        sf::Vector2f mousePos = window.mapPixelToCoords(sf::Mouse::getPosition(window));

        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) window.close();

            if (currentState == GameState::Menu) {
                GameState nextState = menu.handleEvent(event, mousePos);
                if (nextState != GameState::Menu) currentState = nextState;
            } else if (currentState == GameState::Level1) {
                int status = level1.handleEvents(event, window);
                if (status == 0) {
                    if (level1.getIsWon()) level1Won = true;
                    currentState = GameState::Menu;
                }
            } else if (currentState == GameState::Level2) {
                int status = level2.handleEvents(event, window);
                if (status == 0) {
                    if (level2.getIsWon()) level2Won = true;
                    currentState = GameState::Menu;
                }
            }
        }

        if (currentState == GameState::Menu) {
            menu.update(level1Won, level2Won);
            menu.draw(window);
        } else if (currentState == GameState::Level1) {
            level1.draw(window);
        } else if (currentState == GameState::Level2) {
            level2.draw(window);
        }

        window.display();
    }

    return 0;
}