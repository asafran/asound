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

#include "asound-log.h"

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
const float DEF_LSN_POS[3] = {0.0f, 0.0f, 0.0f};
/// "Скорость передвижения" слушателя по умолчанию
const float DEF_LSN_VEL[3] = {0.0f, 0.0f, 0.0f};
/// Направление слушателя по умолчанию
const float DEF_LSN_ORI[6] = {0.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f};

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

class vsgSound;

/// Скорость воспроизведения источника по умолчанию
const float DEF_SRC_PITCH = 1.0f;

/// Минимальная громкость источника
const int MIN_SRC_VOLUME  = 0;
/// Громкость источника по умолчанию
const int DEF_SRC_VOLUME  = 100;
/// Максимальная громкость источника
const int MAX_SRC_VOLUME  = 100;

/// Положение источника по умолчанию
const float DEF_SRC_POS[3] = {0.0f, 0.0f, 1.0f};
/// "Скорость передвижения" источника по умолчанию
const float DEF_SRC_VEL[3] = {0.0f, 0.0f, 0.0f};

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
    ASound(QString soundname, QObject* parent = Q_NULLPTR);
    /// Деструктор
    ~ASound();

    /// Вернуть громкость
    int getVolume();

    /// Вернуть скорость воспроизведения
    float getPitch();

    /// Вернуть флаг зацикливания
    bool getLoop();

    /// Вернуть положение источника
    void getPosition(float &x, float &y, float &z);

    /// Вернуть "скорость передвижения" источника
    void getVelocity(float &x, float &y, float &z);

    /// Вернуть последнюю ошибку
    QString getLastError();

    /// Играет ли звук
    bool isPlaying();

    /// Остановлен ли звук
    bool isStopped();

    /// Приостановлен ли звук
    bool isPaused();

    /// Длительность звука в секундах
    int getDuration();

    void setLastError(const std::string& value)
    {
        LastError_ = "E - " + QString::fromStdString(value);
        emit lastErrorChanged_(value);
    }


public slots:
    /// Установить громкость
    void setVolume(int volume = 100);

    /// Установить скорости воспроизведения
    void setPitch(float pitch);

    /// Установить зацикливание
    void setLoop(bool loop);

    /// Установить положение
    void setPosition(float x, float y, float z);

    /// Установить "скорость передвижения"
    void setVelocity(float x, float y, float z);

    /// Играть звук
    void play();

    /// Приостановить звук
    void pause();

    /// Остановить звук
    void stop();

signals:

    void lastErrorChanged_(const std::string);

    void notify(const std::string msg);


private slots:
    /// Слот обработки таймера уничтожения у звука блока старта
    void onTimerStartKiller();


private:

    // Можно продолжать работу с файлом
    bool canDo_; ///< Флаг допуска к работе с файлом

    // Можно играть звук
    bool canPlay_; ///< Флаг допуска к воспроизведению звука

    // Имеет-ли файл секцию CUE
    bool canCUE_; ///< Флаг наличия фрагмента CUE

    // Имеет-ли файл секцию LABL
    bool canLABL_; ///< Флаг наличия меток в файле

    // Размер чанка блока date при квази-потоковом воспроизведении
    ALsizei DATA_CHUNK_SIZE;

    // Имя звука
    QString soundName_; ///< Имя файла

    // Последняя ошибка
    QString lastError_; ///< Текс последней ошибки

    // Переменная для хранения файла
    QFile* file_; ///< Контейнер файла

    // Информация формата входного звукового файла
    wave_info_header_t wave_info_header_; ///< Структура информации формата файла [RIFF&&WAVE]

    // Информация о файле .wav
    wave_info_fmt_t wave_info_; ///< Структура информации о файле

    // Секция data в WAVE файле
    wave_info_file_data_t wave_info_file_data_; ///< Структура информации секции data

    // "шапка" списка CUE
    wave_cue_head_t cue_head_; ///< Структура "шапка" CUE

    // Список меток CUE
    QList <wave_cue_data_t>cue_data_; ///< Структура информации списка CUE

    // "шапка" списка меток
    wave_list_head_t list_head_; ///< Структура "шапка" LIST

    // Список меток labels (имя, смещение в секции data)
    QMap<QString, uint64_t> wave_labels_; ///< Список меток

    // Хранилище для data секции (самой музыки) файла .wav
    unsigned char* wavData_[BUFFER_BLOCKS]; ///< Контейнер секций блока данных файла wav

    // Размер каждого из 3-х блоков данных фай
    uint64_t blockSize_[BUFFER_BLOCKS]; ///< Размер блоков данных файла wav

    // Буфер OpenAL
    ALuint  buffer_[BUFFER_BLOCKS]; ///< Буфер OpenAL 3 секции (старт, цикл, остановка)

    // Источник OpenAL
    ALuint  source_; ///< Источник OpenAL

    // Формат аудио (mono8/16 - stereo8/16) OpenAL
    ALenum  format_; ///< Формат аудио (mono8/16 - stereo8/16) OpenAL

    // Громкость
    int sourceVolume_; ///< Громкость

    // Скорости воспроизведения
    ALfloat sourcePitch_; ///< Скорость воспроизведения

    // Флаг зацикливания
    bool sourceLoop_; ///< Флаг зацикливания

    // Положение источника
    ALfloat sourcePosition_[3]; ///< Положение источника

    // "Скорость передвижения" источника
    ALfloat sourceVelocity_[3]; ///< "Скорость передвижения" источника

    /// Таймер для стирания в звуке блока старта
    QTimer* timerStartKiller_;

    /// Last error in asound
    QString LastError_;

    /// Полная подготовка файла
    void loadSound_(QString soundname);

    /// Загрузка файла (в т.ч. из ресурсов)
    void loadFile_(QString soundname);

    /// Чтение информации о файле .wav
    void readWaveInfo_();

    /// Чтение формата файла
    void readWaveHeader_();

    /// Чтение данных формата и секции data
    void readWaveFmtData_(QByteArray arr);

    /// Чтение фрагмента LIST ("шапки")
    void readWaveListChunckHeader_(QByteArray &baseStr);

    /// Определение формата аудио (mono8/16 - stereo8/16)
    void defineFormat_();

    /// Получение CUE фрагмента
    void getCUE_(QByteArray &baseStr);

    /// Получение списка меток (Labels)
    void getLabels_(QByteArray &baseStr);

    /// Генерация буфера и источника
    void generateStuff_();

    /// Настройка источника
    void configureSource_();

    /// Метод проверки необходимых параметров
    void checkValue(std::string baseStr, const char targStr[], QString err);

    /// Подпрограмма очистки контейнеров данных дорожки
    void deleteWAVEDataContainers();

    friend class vsgSound;
};

class vsgSound : vsg::Inherit<vsg::Object, vsgSound>
{
public:

    void read(vsg::Input& input) override;
    void write(vsg::Output& output) const override;

    ASound      *sound;
    std::string path;
    int         initVolume;
    float       initPitch;
    bool        loop;
    bool        autostart;
    std::map<double,int>    volumeCurve;
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

#endif // ASOUND_H
