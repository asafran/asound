//-----------------------------------------------------------------------------
//
//      Библиотека для работы со звуком
//      (c) РГУПС, ВЖД 24/03/2017
//      Разработал: Ковшиков С. В.
//
//-----------------------------------------------------------------------------


#include "asound.h"
#include "tools.h"
#include <QFile>
#include <QTimer>

#include "LoadException.h"
#include <vsg/io/Options.h>
#include <vsg/utils/SharedObjects.h>

#include "filesystem.h"

// ****************************************************************************
// *                         Класс AListener                                  *
// ****************************************************************************
//-----------------------------------------------------------------------------
// КОНСТРУКТОР
//-----------------------------------------------------------------------------
AListener::AListener(QObject *parent)
    : QObject(parent)
{
    // Открываем устройство
    _device = alcOpenDevice(nullptr);
    checkAlErrors("Open device");
    // Создаём контекст
    _context = alcCreateContext(_device, nullptr);
    checkAlErrors("Create context");
    // Устанавливаем текущий контекст
    //makeCurrent();
}

AListener::AListener(const vsg::vec3 &position, const vsg::vec3 &up, const vsg::vec3 &at, QObject *parent)
    : AListener(parent)
{
    vsg::vec3 velocity(0.0f, 0.0f, 0.0f);
    // Устанавливаем положение слушателя
    setPosition(position);
    // Устанавливаем скорость слушателя
    setVelocity(vsg::vec3{0.0f, 0.0f, 0.0f});
    // Устанавливаем направление слушателя
    setOrientation(up, at);
}

AListener::~AListener()
{
    closeDevices();
}

void AListener::makeCurrent()
{
    alcMakeContextCurrent(_context);
    checkAlErrors("Make process-wide current context");
}





void AListener::setOrientation(const vsg::vec3 &up, const vsg::vec3 &at)
{
    std::array<float, 6> orientation = {at.x, at.y, at.z,
                                         up.x, up.y, up.z};

    alListenerfv(AL_ORIENTATION, orientation.data());
    checkAlErrors("Set orientation");
}

void AListener::setOrientation(vsg::ref_ptr<vsg::LookAt> lookAt)
{
    setOrientation(static_cast<vsg::vec3>(lookAt->up), static_cast<vsg::vec3>(lookAt->center));
}

void AListener::setPositionOrientation(vsg::ref_ptr<vsg::LookAt> lookAt)
{
    setPosition(lookAt->eye);
    setOrientation(lookAt);
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void AListener::closeDevices()
{
    alcDestroyContext(_context);
    alcCloseDevice(_device);
}

void AListener::makeThreadCurrent()
{
    alcSetThreadContext(_context);
    checkAlErrors("Make thread current context");
}



// ****************************************************************************
// *                            Класс ASound                                  *
// ****************************************************************************
//-----------------------------------------------------------------------------
// КОНСТРУКТОР
//-----------------------------------------------------------------------------
ASound::ASound(QObject *parent) : QObject(parent)
{
    alGenSources(1, &_source);
    checkAlErrors("Created source");
}



//-----------------------------------------------------------------------------
// ДЕСТРУКТОР
//-----------------------------------------------------------------------------
ASound::~ASound()
{
    // Удаляем источник
    alDeleteSources(1, &_source);
}

//-----------------------------------------------------------------------------
// Настройка источника
//-----------------------------------------------------------------------------
void ASound::configureSource()
{
    alSourcei(_source, AL_BUFFER, _buffer->buffer);
    alSourcef(_source, AL_GAIN, _relativeVolume * _volume);
    alSourcef(_source, AL_MIN_GAIN, _minimumVolume);
    alSourcef(_source, AL_MAX_GAIN, _maximumVolume);
    alSourcef(_source, AL_PITCH, _pitch);
    alSourcei(_source, AL_LOOPING, AL_FALSE);
    alSourcef(_source, AL_MAX_DISTANCE, _maxDistance);
    alSourcef(_source, AL_ROLLOFF_FACTOR, _rolloff);
    alSourcef(_source, AL_REFERENCE_DISTANCE, _refDistance);
    alSourcef(_source, AL_CONE_OUTER_GAIN, _coneOuterVolume);
    alSourcef(_source, AL_CONE_INNER_ANGLE, _coneInnerVolume);
    alSourcei(_source, AL_CONE_OUTER_ANGLE, _coneAngle);
    alSourcefv(_source, AL_POSITION, _position.data());
    alSourcefv(_source, AL_DIRECTION, _direction.data());
    alSourcefv(_source, AL_VELOCITY, _velocity.data());
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
std::chrono::milliseconds ASound::getDuration() const
{
    return _buffer->getDuration();
}



//-----------------------------------------------------------------------------
// (слот) Установить громкость
//-----------------------------------------------------------------------------
void ASound::setVolume(float volume)
{
    _volume = volume;
    alSourcef(_source, AL_GAIN, _relativeVolume * _volume);
}


//-----------------------------------------------------------------------------
// (слот) Установить скорость воспроизведения
//-----------------------------------------------------------------------------
void ASound::setPitch(float pitch)
{
    _pitch = pitch;
    alSourcef(_source, AL_PITCH, _pitch);
}

//-----------------------------------------------------------------------------
// (слот) Установить положение
//-----------------------------------------------------------------------------
void ASound::setPosition(const vsg::vec3 &pos)
{
    _position = pos;
    alSourcefv(_source, AL_POSITION, _position.data());
}

//-----------------------------------------------------------------------------
// (слот) Установить направление
//-----------------------------------------------------------------------------

void ASound::setDirection(const vsg::vec3 &dir)
{
    _direction = dir;
    alSourcefv(_source, AL_DIRECTION, _direction.data());
}

//-----------------------------------------------------------------------------
// (слот) Установить вектор "скорости передвижения"
//-----------------------------------------------------------------------------
void ASound::setVelocity(const vsg::vec3 &vel)
{
    _velocity = vel;
    alSourcefv(_source, AL_VELOCITY, _velocity.data());
}

//-----------------------------------------------------------------------------
// (слот) Играть звук
//-----------------------------------------------------------------------------
void ASound::play()
{
    if(isPlaying())
        return;

    alSourcei(_source, AL_LOOPING, AL_FALSE);
    alSourcePlay(_source);
    checkAlErrors("Play sound");
}

void ASound::playLooped()
{
    if(isPlaying())
        return;

    alSourcei(_source, AL_LOOPING, AL_TRUE);
    alSourcePlay(_source);
    checkAlErrors("Play sound");
}

//-----------------------------------------------------------------------------
// (слот) Приостановить звук
//-----------------------------------------------------------------------------
void ASound::pause()
{
    alSourcePause(_source);
}

//-----------------------------------------------------------------------------
// (слот) Остановить звук
//-----------------------------------------------------------------------------
void ASound::stop()
{
    alSourceStop(_source);
}

void ASound::end()
{
    alSourcei(_source, AL_LOOPING, AL_FALSE);
}


//-----------------------------------------------------------------------------
// Играет ли звук
//-----------------------------------------------------------------------------
bool ASound::isPlaying() const
{
    ALint state;
    alGetSourcei(_source, AL_SOURCE_STATE, &state);
    return(state == AL_PLAYING);
}


//-----------------------------------------------------------------------------
// Приостановлен ли звук
//-----------------------------------------------------------------------------
bool ASound::isPaused() const
{
    ALint state;
    alGetSourcei(_source, AL_SOURCE_STATE, &state);
    return(state == AL_PAUSED);
}


//-----------------------------------------------------------------------------
// Остановлен ли звук
//-----------------------------------------------------------------------------
bool ASound::isStopped() const
{
    ALint state;
    alGetSourcei(_source, AL_SOURCE_STATE, &state);
    return(state == AL_STOPPED);
}

vsg::ref_ptr<ABuffer> ASound::buffer() const
{
    return _buffer;
}

void ASound::setBuffer(vsg::ref_ptr<ABuffer> newBuffer)
{
    stop();
    _buffer = newBuffer;
    alSourcei(_source, AL_BUFFER, _buffer->buffer);
    checkAlErrors("Set new buffer");
}

float ASound::volume() const
{
    return _volume;
}

float ASound::minimumVolume() const
{
    return _minimumVolume;
}

void ASound::setMinimumVolume(float newMinimumVolume)
{
    _minimumVolume = newMinimumVolume;
    alSourcef(_source, AL_MIN_GAIN, _minimumVolume);
}

float ASound::maximumVolume() const
{
    return _maximumVolume;
}

void ASound::setMaximumVolume(float newMaximumVolume)
{
    _maximumVolume = newMaximumVolume;
    alSourcef(_source, AL_MAX_GAIN, _maximumVolume);
}

void ASound::setRealativeVolume(float newRealativeVolume)
{
    _relativeVolume = newRealativeVolume;
    alSourcef(_source, AL_GAIN, _relativeVolume * _volume);
}

float ASound::pitch() const
{
    return _pitch;
}

bool ASound::isLooped() const
{
    ALint looped;
    alGetSourcei(_source, AL_LOOPING, &looped);
    return(looped == AL_TRUE);
}

float ASound::maxDistance() const
{
    return _maxDistance;
}

void ASound::setMaxDistance(float newMaxDistance)
{
    _maxDistance = newMaxDistance;
    alSourcef(_source, AL_MAX_DISTANCE, _maxDistance);
}

float ASound::rolloff() const
{
    return _rolloff;
}

void ASound::setRolloff(float newNrolloff)
{
    _rolloff = newNrolloff;
    alSourcef(_source, AL_ROLLOFF_FACTOR, _rolloff);
}

float ASound::refDistance() const
{
    return _refDistance;
}

void ASound::setRefDistance(float newNrefDistance)
{
    _refDistance = newNrefDistance;
    alSourcef(_source, AL_REFERENCE_DISTANCE, _refDistance);
}

float ASound::coneOuterVolume() const
{
    return _coneOuterVolume;
}

void ASound::setConeOuterVolume(float newConeOuterVolume)
{
    _coneOuterVolume = newConeOuterVolume;
    alSourcef(_source, AL_CONE_OUTER_GAIN, _coneOuterVolume);
}

float ASound::coneInnerVolume() const
{
    return _coneInnerVolume;
}

void ASound::setConeInnerVolume(float newConeInnerVolume)
{
    _coneInnerVolume = newConeInnerVolume;
    alSourcef(_source, AL_CONE_INNER_ANGLE, _coneInnerVolume);
}

int ASound::coneAngle() const
{
    return _coneAngle;
}

void ASound::setConeAngle(int newConeAngle)
{
    _coneAngle = newConeAngle;
    alSourcei(_source, AL_CONE_OUTER_ANGLE, _coneAngle);
}

const vsg::vec3 &ASound::position() const
{
    return _position;
}

const vsg::vec3 &ASound::velocity() const
{
    return _velocity;
}

const vsg::vec3 &ASound::direction() const
{
    return _direction;
}



//-----------------------------------------------------------------------------
//
//      Класс управления очередью запуска звуков
//      (c) РГУПС, ВЖД 17/08/2017
//      Разработал: Ковшиков С. В.
//
//-----------------------------------------------------------------------------
/*!
 *  \file
 *  \brief Класс управления очередью запуска звуков
 *  \copyright РУГПС, ВЖД
 *  \author Ковшиков С. В.
 *  \date 17/08/2017
 */

/*
//-----------------------------------------------------------------------------
// Конструктор
//-----------------------------------------------------------------------------
ASoundController::ASoundController(QObject *parent)
    : QObject(parent)
    , prepared_(false)
    , beginning_(false)
    , running_(false)
    , currentSoundIndex_(0)
    , soundPitch_(1.0f)
    , soundVolume_(100)
    , soundBegin_(Q_NULLPTR)
    , soundEnd_(Q_NULLPTR)
    , timerSoundChanger_(Q_NULLPTR)
{
    timerSoundChanger_ = new QTimer(this);
    timerSoundChanger_->setSingleShot(true);
    connect(timerSoundChanger_, SIGNAL(timeout()),
            this, SLOT(onTimerSoundChanger()));
}



//-----------------------------------------------------------------------------
// Деструктор
//-----------------------------------------------------------------------------
ASoundController::~ASoundController()
{

}



//-----------------------------------------------------------------------------
// Установить звук запуска
//-----------------------------------------------------------------------------
void ASoundController::setSoundBegin(QString soundPath)
{
    prepared_ = false;

    ASound* buf = new ASound(soundPath, this);

    if (buf->getLastError().isEmpty())
    {
        if (soundBegin_)
            delete soundBegin_;
        soundBegin_ = buf;
        soundBegin_->setVolume(soundVolume_);
    }
    else
    {
        delete buf;
    }

    prepare_();
}



//-----------------------------------------------------------------------------
// Добавить звук процесса работы
//-----------------------------------------------------------------------------
void ASoundController::appendSoundRunning(QString soundPath)
{
    prepared_ = false;

    ASound* buf = new ASound(soundPath, this);

    if (buf->getLastError().isEmpty())
    {
        listRunningSounds_.append(buf);
        listRunningSounds_.last()->setLoop(true);
    }
    else
    {
        delete buf;
    }

    prepare_();
}



//-----------------------------------------------------------------------------
// Установить список звуков процесса работы
//-----------------------------------------------------------------------------
void ASoundController::setSoundsRunning(QStringList soundPaths)
{
    prepared_ = false;

    clearRunningSoundsList_();

    for (QString path : soundPaths)
    {
        ASound* buf = new ASound(path, this);

        if (buf->getLastError().isEmpty())
        {
            listRunningSounds_.append(buf);
            listRunningSounds_.last()->setLoop(true);
        }
        else
        {
            delete buf;
        }
    }

    prepare_();
}



//-----------------------------------------------------------------------------
// Установить звук остановки
//-----------------------------------------------------------------------------
void ASoundController::setSoundEnd(QString soundPath)
{
    prepared_ = false;

    ASound* buf = new ASound(soundPath, this);

    if (buf->getLastError().isEmpty())
    {
        if (soundEnd_)
            delete soundEnd_;
        soundEnd_ = buf;
        soundEnd_->setVolume(soundVolume_);
    }

    prepare_();
}



//-----------------------------------------------------------------------------
// Запустить алгоритм воспроизведения (запуск устройства)
//-----------------------------------------------------------------------------
void ASoundController::begin()
{
    if (prepared_ && !running_ && !beginning_)
    {
        beginning_ = true;

        timerSoundChanger_->start();
        soundBegin_->play();
    }
}



//-----------------------------------------------------------------------------
// Установить звук процесса работы
//-----------------------------------------------------------------------------
void ASoundController::switchRunningSound(int index)
{
    if (running_)
    {
        if (index < listRunningSounds_.count() && index != currentSoundIndex_)
        {
            ASound* buf = listRunningSounds_[index];
            buf->setPitch(soundPitch_);
            buf->setVolume(soundVolume_);
            buf->play();
            listRunningSounds_[currentSoundIndex_]->stop();
            currentSoundIndex_ = index;
        }
    }
}



//-----------------------------------------------------------------------------
// Завершить алгоритм воспроизведения (остановка устройства)
//-----------------------------------------------------------------------------
void ASoundController::end()
{
    if (running_ || beginning_)
    {
        timerSoundChanger_->stop();
        soundEnd_->play();
        soundBegin_->stop();
        listRunningSounds_[currentSoundIndex_]->stop();
        beginning_ = false;
        running_ = false;
    }
}



//-----------------------------------------------------------------------------
// Установить скорость воспроизведения
//-----------------------------------------------------------------------------
void ASoundController::setPitch(float pitch)
{
    soundPitch_ = pitch;

    if (running_)
    {
        listRunningSounds_[currentSoundIndex_]->setPitch(pitch);
    }
}



//-----------------------------------------------------------------------------
// Установить громкость 0 - 100
//-----------------------------------------------------------------------------
void ASoundController::setVolume(int volume)
{
    soundVolume_ = volume;

    if (soundBegin_)
    {
        soundBegin_->setVolume(volume);
    }

    if (soundEnd_)
    {
        soundEnd_->setVolume(volume);
    }

    if (running_)
    {
        listRunningSounds_[currentSoundIndex_]->setVolume(volume);
    }
}



//-----------------------------------------------------------------------------
// Аварийно завершить алгоритм вопсроизведения в любой момент
//-----------------------------------------------------------------------------
void ASoundController::forcedStop()
{
    timerSoundChanger_->stop();
    soundBegin_->stop();
    running_ = false;
    for (ASound* sound : listRunningSounds_)
    {
        sound->stop();
    }
}



//-----------------------------------------------------------------------------
// Слот обработки таймера переключения звуков
//-----------------------------------------------------------------------------
void ASoundController::onTimerSoundChanger()
{
    currentSoundIndex_ = 0;
    ASound* buf = listRunningSounds_[currentSoundIndex_];
    buf->setPitch(soundPitch_);
    buf->setVolume(soundVolume_);
    buf->play();
    beginning_ = false;
    running_ = true;
}



//-----------------------------------------------------------------------------
// Проверить готовность всех звуков
//-----------------------------------------------------------------------------
void ASoundController::prepare_()
{
    if ( (soundBegin_ != Q_NULLPTR) &&
         (soundEnd_ != Q_NULLPTR) &&
         (listRunningSounds_.count() > 0) )
    {
        timerSoundChanger_->setInterval(soundBegin_->getDuration());
        prepared_ = true;
    }
}



//-----------------------------------------------------------------------------
// Очистить список фаз процесса работы
//-----------------------------------------------------------------------------
void ASoundController::clearRunningSoundsList_()
{
    if (listRunningSounds_.isEmpty())
        return;

    for (ASound* sound : listRunningSounds_)
    {
        delete sound;
    }

    listRunningSounds_.clear();
}*/

void vsgSound::read(vsg::Input &input)
{
    vsg::Object::read(input);

    sound = new ASound();
    input.read("name", sound->soundName);

    bool streamed = false;
    input.read("streamed", streamed);

    if(streamed)
    {
        sound->_buffer = ABuffer::loadStreamed(sound->soundName);
    }
    else
    {
        auto loadedObject = vsg::LoadedObject::create(sound->soundName, input.options);
        input.options->sharedObjects->share(loadedObject, [&](auto load) {
            load->object = ABuffer::loadFull(sound->soundName);
        });
        sound->_buffer = loadedObject->object.cast<ABuffer>();
    }

    input.read("vol", sound->_volume);
    input.read("minVol", sound->_minimumVolume);
    input.read("maxVol", sound->_maximumVolume);
    input.read("pitch", sound->_pitch);
    input.read("maxDistance", sound->_maxDistance);
    input.read("rolloff", sound->_rolloff);
    input.read("refDistance", sound->_refDistance);
    input.read("outerVol", sound->_coneOuterVolume);
    input.read("innerVol", sound->_coneInnerVolume);
    input.read("coneAngle", sound->_coneAngle);
    input.read("position", sound->_position);
    input.read("direction", sound->_direction);

    tools::readMap(input, "volKeyPoint", sound->volumeCurve);
}

void vsgSound::write(vsg::Output &output) const
{
    vsg::Object::write(output);

    output.write("name", sound->soundName);
    bool streamed = sound->_buffer->streamed();
    output.write("streamed", streamed);
    output.write("vol", sound->_volume);
    output.write("minVol", sound->_minimumVolume);
    output.write("maxVol", sound->_maximumVolume);
    output.write("pitch", sound->_pitch);
    output.write("maxDistance", sound->_maxDistance);
    output.write("rolloff", sound->_rolloff);
    output.write("refDistance", sound->_refDistance);
    output.write("outerVol", sound->_coneOuterVolume);
    output.write("innerVol", sound->_coneInnerVolume);
    output.write("coneAngle", sound->_coneAngle);
    output.write("position", sound->_position);
    output.write("direction", sound->_direction);

    tools::writeMap(output, "volKeyPoint", sound->volumeCurve);
}


ABuffer::ABuffer() : vsg::Inherit<vsg::Object, ABuffer>()
{
    alGenBuffers(1, &buffer);
    checkAlErrors("Generating buffer");
}

ABuffer::~ABuffer()
{
    alDeleteBuffers(1, &buffer);
    if(_file) op_free(_file);
}

vsg::ref_ptr<ABuffer> ABuffer::loadFull(const std::string &path)
{
    auto buf = ABuffer::create();

    auto &fs = FileSystem::getInstance();
    auto pathToFile = fs.getSoundPath(path);

    int error = 0;
    auto file = op_open_file(pathToFile.c_str(), &error);
    if(error < 0)
        throw LoadException(QString("Failed to load %1 sound").arg(pathToFile.c_str()));

    int links = op_link_count(file);

    std::vector<ALint> blocks;
    ALint offset = 0;
    for (int li = 0; li < links; ++li) {
        auto channels = op_channel_count(file, li);
        if(channels > 1)
            throw LoadException(QString("Failed to load %1 sound, stereo sound not allowed").arg(pathToFile.c_str()));
        offset += op_raw_total(file, li);
        blocks.at(li) = offset;
    }

    auto total = op_pcm_total(file, -1);
    opus_int16 *pcm = new opus_int16[total];
    error = op_read(file, pcm, total, nullptr);
    if(error < 0)
        throw LoadException(QString("Failed to load pcm %1 sound").arg(pathToFile.c_str()));

    alBufferData(buf->buffer, AL_FORMAT_MONO16, pcm, total, OPUS_DECODE_SAPLE_RATE);
    checkAlErrors("Fill buffer data");

    if(links == 3)
    {
        alBufferiv(buf->buffer, AL_LOOP_POINTS_SOFT, blocks.data());
        checkAlErrors("Set looping points");
    }

    delete [] pcm;
    op_free(file);

    return buf;
}

vsg::ref_ptr<ABuffer> ABuffer::loadStreamed(const std::string &path)
{
    auto buf = ABuffer::create();

    auto &fs = FileSystem::getInstance();
    auto pathToFile = fs.getSoundPath(path);

    int error = 0;
    buf->_file = op_open_file(pathToFile.c_str(), &error);
    if(error < 0)
        throw LoadException(QString("Failed to load %1 sound").arg(pathToFile.c_str()));

    int links = op_link_count(buf->_file);
    for (int li = 0; li < links; ++li) {
        auto channels = op_channel_count(buf->_file, li);
        if(channels > 1)
            throw LoadException(QString("Failed to load %1 sound, stereo sound not allowed").arg(pathToFile.c_str()));
    }
    auto callback = [](ALvoid *userptr, ALvoid *sampleData, ALsizei numbytes)
            -> ALsizei
    {
        auto samples = numbytes / sizeof(opus_int16);
        auto file = static_cast<OggOpusFile*>(sampleData);
        if(!file)
            return 0;
        auto written = op_read(file, static_cast<short*>(sampleData), samples, NULL);
        written *= sizeof(opus_int16);
        return std::max(static_cast<ALsizei>(written), 0);
    };
    alBufferCallbackSOFT(buf->buffer, AL_FORMAT_MONO16, OPUS_DECODE_SAPLE_RATE, callback, buf->_file);

    return buf;
}
/*
int ABuffer::loadBlock(ALuint to)
{
    if(!_file)
        return -1;
    opus_int16 *pcm = new opus_int16[bufferSize];

    auto samples = op_read(_file, pcm, bufferSize, nullptr);
    if(samples <= 0)
        return samples;

    alBufferData(to, AL_FORMAT_MONO16, pcm, samples, OPUS_DECODE_SAPLE_RATE);
    checkAlErrors("Fill buffer data");

    delete [] pcm;
    return samples;
}
*/
std::chrono::milliseconds ABuffer::getDuration() const
{
    auto total = op_pcm_total(_file, -1);
    return std::chrono::milliseconds(total / (OPUS_DECODE_SAPLE_RATE / 1000));
}

bool ABuffer::streamed() const
{
    return _file != nullptr;
}
