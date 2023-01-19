//-----------------------------------------------------------------------------
//
//      Библиотека для работы с 3D звуком
//      (c) РГУПС, ВЖД 24/03/2017
//      Разработал: Ковшиков С. В.
//
//-----------------------------------------------------------------------------
/*!
 *  \file
 *  \brief Библиотека для работы с 3D звуком
 *  \copyright РГУПС, ВЖД
 *  \author Ковшиков С. В.
 *  \date 24/03/2017
 */

#ifndef ASOUND_H
#define ASOUND_H

#include <QObject>
#include <QMap>
#include <AL/al.h>
#include <AL/alc.h>

#include <vsg/app/ViewMatrix.h>
#include <vsg/maths/transform.h>

#include "Journal.h"

#include <opusfile.h>

class QFile;
class QTimer;

#if defined(ASOUND_LIBRARY)
#  define ASOUNDSHARED_EXPORT Q_DECL_EXPORT
#else
#  define ASOUNDSHARED_EXPORT Q_DECL_IMPORT
#endif

#define BUFFER_BLOCKS 3


//-----------------------------------------------------------------------------
// Класс AListener
//-----------------------------------------------------------------------------
/// Положение слушателя по умолчанию
//const float DEF_LSN_POS[3] = {0.0f, 0.0f, 0.0f};
/// "Скорость передвижения" слушателя по умолчанию
//const float DEF_LSN_VEL[3] = {0.0f, 0.0f, 0.0f};
/// Направление слушателя по умолчанию
//const float DEF_LSN_ORI[6] = {0.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f};

static void checkAlErrors(QString msg)
{
    //Journal::instance()->debug("ASound: " + msg);
    ALenum error = alGetError();
    if(error != ALC_NO_ERROR)
        Journal::instance()->error("OpenAL error: " + QString::number(error));
}

constexpr int OPUS_DECODE_SAPLE_RATE = 48000;

/*!
 * \class AListener
 * \brief Класс, реализующий создание единственного слушателя
 */
class ASOUNDSHARED_EXPORT AListener : public QObject
{   
    Q_OBJECT
public:

    /// Конструктор
    AListener(QObject *parent = nullptr);
    AListener(const vsg::vec3 &position, const vsg::vec3 &up, const vsg::vec3 &at, QObject *parent = nullptr);

    virtual ~AListener();

    void makeCurrent();

    template<typename T>
    void setPosition(const vsg::t_vec3<T> &position)
    {
        alListenerfv(AL_POSITION,    static_cast<vsg::vec3>(position).data());
        checkErrors();
    }

    template<typename T>
    void setVelocity(const vsg::t_vec3<T> &velocity)
    {
        alListenerfv(AL_VELOCITY,   static_cast<vsg::vec3>(velocity).data());
        checkErrors();
    }
    void setOrientation(const vsg::vec3 &up, const vsg::vec3 &at);
    void setOrientation(vsg::ref_ptr<vsg::LookAt> lookAt);

    void setPositionOrientation(vsg::ref_ptr<vsg::LookAt> lookAt);

    ///
    void closeDevices();

signals:
    void logMsg(QString msg);

private:
    void checkErrors();

    /// Аудиоустройство
    ALCdevice* _device;

    /// Контекст OpenAL
    ALCcontext* _context;
/*
    /// Положение слушателя
    ALfloat listenerPosition_[3];

    /// "Скорость передвижения" слушателя
    ALfloat listenerVelocity_[3];

    /// Направление слушателя
    std::array<float, 6> _orientation;
*/
};



//-----------------------------------------------------------------------------
// Класс ASound
//-----------------------------------------------------------------------------

class ABuffer : public vsg::Inherit<vsg::Object, ABuffer>
{
public:
    ABuffer();
    virtual ~ABuffer();

    static vsg::ref_ptr<ABuffer> loadFull(const std::string &path);
    static vsg::ref_ptr<ABuffer> loadStreamed(const std::string &path, size_t dynBuffers, int bufferSize);

    int loadBlock(ALuint to);
    void initStream();

    std::chrono::milliseconds getDuration() const;

    bool streamed() const;

    // Буфер OpenAL
    std::vector<ALuint> buffers;
    int bufferSize = 200000;

private:
    // Ogg файл, null если буфер статический
    OggOpusFile *_file = nullptr;
};

class vsgSound;

/*!
 * \class ASound
 * \brief Класс реализующий создание источника звука, настройки его
 * пространственных характеристик, загрузки аудиофайла в формате wav и
 * последующего воспроизведения
 */
class ASOUNDSHARED_EXPORT ASound : public QObject
{
    Q_OBJECT
    Q_PROPERTY(std::string lastError_ WRITE setLastError NOTIFY lastErrorChanged_)
public:
    /*!
     * \brief Конструктор
     * \param soundname - имя аудиофайла
     */
    explicit ASound(QObject* parent = Q_NULLPTR);
    /// Деструктор
    virtual ~ASound();

    /// Играет ли звук
    bool isPlaying() const;

    /// Остановлен ли звук
    bool isStopped() const;

    /// Приостановлен ли звук
    bool isPaused() const;

    /// Играет ли звук в цикле
    bool isLooped() const;

    /// Длительность звука в секундах
    std::chrono::milliseconds getDuration() const;

    std::string soundName; ///< Имя файла

    std::map<double,int>    volumeCurve;

    vsg::ref_ptr<ABuffer> buffer() const;
    void setBuffer(vsg::ref_ptr<ABuffer> newBuffer);
    float volume() const;
    float minimumVolume() const;
    void setMinimumVolume(float newMinimumVolume);
    float maximumVolume() const;
    void setMaximumVolume(float newMaximumVolume);
    void setRealativeVolume(float newRealativeVolume);
    float pitch() const;

    float maxDistance() const;
    void setMaxDistance(float newMaxDistance);
    float rolloff() const;
    void setRolloff(float newNrolloff);
    float refDistance() const;
    void setRefDistance(float newNrefDistance);
    float coneOuterVolume() const;
    void setConeOuterVolume(float newConeOuterVolume);
    float coneInnerVolume() const;
    void setConeInnerVolume(float newConeInnerVolume);
    const vsg::vec3 &position() const;
    const vsg::vec3 &velocity() const;
    const vsg::vec3 &direction() const;
    int coneAngle() const;
    void setConeAngle(int newConeAngle);

public slots:
    /// Установить громкость
    void setVolume(float volume);

    /// Установить скорости воспроизведения
    void setPitch(float pitch);

    /// Установить положение
    void setPosition(const vsg::vec3 &pos);

    /// Установить направление
    void setDirection(const vsg::vec3 &dir);

    /// Установить "скорость передвижения"
    void setVelocity(const vsg::vec3 &vel);

    /// Играть звук
    void play();

    /// Играть в цикле
    void playLooped();

    /// Приостановить звук
    void pause();

    /// Остановить звук
    void stop();

    /// Завершить звук
    void end();

protected:
    void timerEvent(QTimerEvent *event) override;

private:

    vsg::ref_ptr<ABuffer> _buffer; /// Буффер OpenAL

    ALuint  _source; ///< Источник OpenAL

    float _volume = 1.0f; ///< Громкость

    float _minimumVolume = 0.0f; ///< Минимальная громкость

    float _maximumVolume = 1.0f; ///< Максимальная громкость

    float _relativeVolume = 1.0f; ///< Регулировка

    float _pitch = 1.0f; ///< Скорость воспроизведения

    bool _looped = false; ///< Флаг зацикливания

    float _maxDistance = 10.0f; ///< Максимальное расстояние, на которое распространяется звук

    float _rolloff = 1.0f; ///< Распространение звука в пространстве

    float _refDistance = 5.0f; ///< Расстояние на котором громкость звука будет составлять половину от нормальной

    float _coneOuterVolume = 0.3f; ///< Громкость за пределами направления звука

    float _coneInnerVolume = 1.0f; ///< Громкость "внутри" звука

    int _coneAngle = 360; ///< Угол звука

    vsg::vec3 _position; ///< Положение источника

    vsg::vec3 _velocity; ///< "Скорость передвижения" источника

    vsg::vec3 _direction; ///< Направление звука

    /// Настройка источника
    void configureSource();

    void requeueBuffers();

    friend class vsgSound;
};

class vsgSound : public vsg::Inherit<vsg::Object, vsgSound>
{
public:

    void read(vsg::Input& input) override;
    void write(vsg::Output& output) const override;

    ASound      *sound;
};



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

/*!
 * \brief Класс управления очередью запуска звуков
 * \class ASoundController
 */
/*
class ASOUNDSHARED_EXPORT ASoundController : public QObject
{
    Q_OBJECT

public:
    /// Конструктор
    ASoundController(QObject* parent = Q_NULLPTR);
    /// Деструктор
    ~ASoundController();

    /// Установить звук запуска
    void setSoundBegin(QString soundPath);

    /// Добавить звук процесса работы
    void appendSoundRunning(QString soundPath);

    /// Установить список звуков процесса работы
    void setSoundsRunning(QStringList soundPaths);

    /// Установить звук остановки
    void setSoundEnd(QString soundPath);


public slots:
    /// Запустить алгоритм воспроизведения (запуск устройства)
    void begin();

    /// Установить звук процесса работы
    void switchRunningSound(int index);

    /// Завершить алгоритм воспроизведения (остановка устройства)
    void end();

    /// Установить скорость воспроизведения
    void setPitch(float pitch);

    /// Установить громкость 0 - 100
    void setVolume(int volume);

    /// Аварийно завершить алгоритм вопсроизведения в любой момент
    void forcedStop();


private slots:
    /// Слот обработки таймера переключения звуков
    void onTimerSoundChanger();


private:
    /// Флаг готовности
    bool prepared_;

    /// Флаг запуска устройства
    bool beginning_;

    /// Флаг проигрывания
    bool running_;

    /// Индекс текущей фазы звука
    int currentSoundIndex_;

    /// Скорость воспроизведения
    float soundPitch_;

    /// Громкость воспроизведения
    int soundVolume_;

    /// Звук включения системы
    ASound* soundBegin_;

    /// Список фаз процесса работы
    QList<ASound*> listRunningSounds_;

    /// Звук выключения системы
    ASound* soundEnd_;

    /// Таймер перехода от включения к процессу работы
    QTimer* timerSoundChanger_;

    /// Проверить готовность всех звуков
    void prepare_();

    /// Очистить список фаз процесса работы
    void clearRunningSoundsList_();
};
*/
#endif // ASOUND_H
